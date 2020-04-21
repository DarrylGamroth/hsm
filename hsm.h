#ifndef HSM_HPP
#define HSM_HPP

// This code is from:
// Yet Another Hierarchical State Machine
// by Stefan Heinzmann
// Overload issue 64 december 2004
// https://www.state-machine.com/doc/Heinzmann04.pdf

/* This is a basic implementation of UML Statecharts.
 * The key observation is that the machine can only
 * be in a leaf state at any given time. The composite
 * states are only traversed, never final.
 * Only the leaf states are ever instantiated. The composite
 * states are only mechanisms used to generate code. They are
 * never instantiated.
 */

#include <type_traits>

namespace hsm
{

template <typename THost>
struct TopState
{
    using Host = THost;
    using Base = void;

    virtual void handler(Host &) const = 0;
    virtual unsigned getId() const = 0;
};

template <typename THost, unsigned id, typename TBase>
struct CompState;

template <typename THost, unsigned id,
          typename TBase = CompState<THost, 0, TopState<THost>>>
struct CompState : TBase
{
    using Base = TBase;
    using This = CompState<THost, id, Base>;

    template <typename T>
    void handle(THost &host, const T &t) const
    {
        Base::handle(host, t);
    }

    // no implementation
    static void initial(THost &);

    static void entry(THost &) {}

    static void exit(THost &) {}
};

// Class used for Top state
template <typename THost>
struct CompState<THost, 0, TopState<THost>> : TopState<THost>
{
    using Base = TopState<THost>;
    using This = CompState<THost, 0, Base>;

    template <typename T>
    void handle(THost &, const T &) const {}

    // no implementation
    static void initial(THost &);

    static void entry(THost &) {}

    static void exit(THost &) {}
};

// Define the leaf state
template <typename THost, unsigned id,
          typename TBase = CompState<THost, 0, TopState<THost>>>
struct LeafState : TBase
{
    using Host = THost;
    using Base = TBase;
    using This = LeafState<THost, id, Base>;

    template <typename T>
    void handle(THost &host, const T &t) const
    {
        Base::handle(host, t);
    }

    virtual void handler(THost &host) const { handle(host, *this); }

    virtual unsigned getId() const { return id; }

    static void initial(THost &host) { host.next(obj); }

    // don't specialize this
    static void entry(THost &) {}

    static void exit(THost &) {}

    LeafState() {}

private:
    static const LeafState obj; // only the leaf states have instances
};

template <typename THost, unsigned id, typename TBase>
const LeafState<THost, id, TBase> LeafState<THost, id, TBase>::obj;

// Transition Object, Current, Source, Target
template <typename TCurrent, typename TSource, typename TTarget>
struct Tran
{
    using Host = typename TCurrent::Host;
    using CurrentBase = typename TCurrent::Base;
    using SourceBase = typename TSource::Base;
    using TargetBase = typename TTarget::Base;

    enum
    {
        // work out when to terminate template recursion
        eCB_TB = std::is_base_of<CurrentBase, TargetBase>::value,
        eCB_S = std::is_base_of<CurrentBase, TSource>::value,
        eC_S = std::is_base_of<TCurrent, TSource>::value,
        eS_C = std::is_base_of<TSource, TCurrent>::value,
        exitStop = eCB_TB && eC_S,
        entryStop = eC_S || (eCB_S && !eS_C)
    };

    // We use overloading to stop recursion.
    // The more natural template specialization
    // method would require to specialize the inner
    // template without specializing the outer one,
    // which is forbidden.
    static void exitActions(Host &, std::true_type) {}

    static void exitActions(Host &host, std::false_type)
    {
        TCurrent::exit(host);
        Tran<CurrentBase, TSource, TTarget>::exitActions(host, std::bool_constant<exitStop>());
    }

    static void entryActions(Host &, std::true_type) {}

    static void entryActions(Host &host, std::false_type)
    {
        Tran<CurrentBase, TSource, TTarget>::entryActions(host, std::bool_constant<entryStop>());
        TCurrent::entry(host);
    }

    Tran(Host &host) : host_{host} { exitActions(host_, std::bool_constant<false>()); }

    ~Tran()
    {
        Tran<TTarget, TSource, TTarget>::entryActions(host_, std::bool_constant<false>());
        TTarget::initial(host_);
    }

    Host &host_;
};

// Initializer for Compound States
template <typename T>
struct Initial
{
    using Host = typename T::Host;

    Initial(Host &host) : host_{host} {}

    ~Initial()
    {
        T::entry(host_);
        T::initial(host_);
    }

    Host &host_;
};

template <typename T>
class HSM
{
public:
    const TopState<T> *state_;

    void next(const TopState<T> &state) { state_ = &state; }
};

} // namespace hsm
#endif // HSM_HPP
