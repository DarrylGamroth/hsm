#ifndef HSM_HPP
#define HSM_HPP

// This code is from:
// Yet Another Hierarchical State Machine
// by Stefan Heinzmann
// Overload issue 64 December 2004
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

/**
 * @brief Abstract class for state
 *
 * @tparam THost host state machine class
 */
template <typename THost>
struct HsmState
{
    using Host = THost;
    using Base = void;

    /**
     * @brief Event handler
     *
     * @param host
     */
    virtual void handler(Host &host) const = 0;

    /**
     * @brief Get the Id object
     *
     * @return unsigned unique numerical id
     */
    virtual unsigned getId() const = 0;
};

/**
 * @brief Forward declaration of Composite state
 *
 * @tparam THost host state machine class
 * @tparam id unique numerical id
 * @tparam TBase base state
 */
template <typename THost, unsigned id, typename TBase>
struct CompositeState;

/**
 * @brief Composite state class
 *
 * @tparam THost host state machine class
 * @tparam id unique numerical id
 * @tparam TBase base state
 */
template <typename THost, unsigned id,
          typename TBase = CompositeState<THost, 0, HsmState<THost>>>
struct CompositeState : TBase
{
    using Host = THost;
    using Base = TBase;
    using This = CompositeState<Host, id, Base>;

    /**
     * @brief Default event handler
     * 
     * @tparam T 
     * @param host 
     * @param x 
     */
    template <typename T>
    void handle(THost &host, const T &x) const
    {
        Base::handle(host, x);
    }

    /**
     * @brief Initial state transition
     *
     * No implementation so compiler can catch if an initial transition
     * is not implemented
     */
    static void initial(THost &);

    /**
     * @brief Default state entry handler
     * 
     * @param host 
     */
    static void entry(THost &host) {}

    /**
     * @brief Default state exit handler
     * 
     * @param host 
     */
    static void exit(THost &host) {}
};

/**
 * @brief Composite state class for top state
 *
 * @tparam THost
 */
template <typename THost>
struct CompositeState<THost, 0, HsmState<THost>> : HsmState<THost>
{
    using Host = THost;
    using Base = HsmState<Host>;
    using This = CompositeState<Host, 0, Base>;

    /**
     * @brief Default event handler
     * 
     * @tparam T 
     * @param host 
     * @param x 
     */
    template <typename T>
    void handle(THost &, const T &) const {}

    /**
     * @brief Initial state transition
     *
     * No implementation so compiler can catch if an initial transition
     * is not implemented
     */
    static void initial(THost &);

    /**
     * @brief Default state entry handler
     * 
     * @param host 
     */
    static void entry(THost &host) {}

    /**
     * @brief Default state exit handler
     * 
     * @param host 
     */
    static void exit(THost &host) {}
};

/**
 * @brief Leaf state class. Only leaf states have instances.
 *
 * @tparam THost host state machine class
 * @tparam id unique numerical id
 * @tparam TBase base state
 */
template <typename THost, unsigned id,
          typename TBase = CompositeState<THost, 0, HsmState<THost>>>
struct LeafState : TBase
{
    using Host = THost;
    using Base = TBase;
    using This = LeafState<Host, id, Base>;

    /**
     * @brief Default event handler
     * 
     * @tparam T 
     * @param host 
     * @param x 
     */
    template <typename T>
    void handle(THost &host, const T &x) const
    {
        Base::handle(host, x);
    }

    /**
     * @brief Event handler called by host
     *
     * @param host
     */
    virtual void handler(THost &host) const override { handle(host, *this); }

    /**
     * @brief Get the Id object
     *
     * @return unsigned unique numerical id
     */
    virtual unsigned getId() const override { return id; }

    /**
     * @brief Initial transition. No specialization in required as the
     * transition is defined for leaf nodes.
     *
     * @param host
     */
    static void initial(THost &host) { host.next(obj_); }

    /**
     * @brief Default state entry handler
     * 
     * @param host 
     */
    static void entry(THost &host) {}

    /**
     * @brief Default state exit handler
     * 
     * @param host 
     */
    static void exit(THost &host) {}

    LeafState(LeafState const &) = delete;
    LeafState &operator=(LeafState const &) = delete;

private:
    LeafState() = default;

    static const LeafState obj_;
};

/**
 * @brief Leaf state instance 
 * 
 * @tparam THost host state machine class
 * @tparam id unique numerical id
 * @tparam TBase base state
 */
template <typename THost, unsigned id, typename TBase>
const LeafState<THost, id, TBase> LeafState<THost, id, TBase>::obj_;

/**
 * @brief Transition object
 *
 * @tparam TCurrent
 * @tparam TSource
 * @tparam TTarget
 */
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
        eCB_TB = std::is_base_of<CurrentBase, TargetBase>(),
        eCB_S = std::is_base_of<CurrentBase, TSource>(),
        eC_S = std::is_base_of<TCurrent, TSource>(),
        eS_C = std::is_base_of<TSource, TCurrent>(),

        /*
         * Tran now needs to walk up in the inheritance hierarchy of
         * the current state (TCurrent) until it finds the common base class
         * of current and target state (TCurrent and TTarget), but it must not
         * stop before the source state (TSource) was reached.
         */
        exit_stop = eCB_TB && eC_S,
        entry_stop = eC_S || (eCB_S && !eS_C)
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
        Tran<CurrentBase, TSource, TTarget>::exitActions(
            host, std::bool_constant<exit_stop>());
    }

    static void entryActions(Host &, std::true_type) {}

    static void entryActions(Host &host, std::false_type)
    {
        Tran<CurrentBase, TSource, TTarget>::entryActions(
            host, std::bool_constant<entry_stop>());
        TCurrent::entry(host);
    }

    /**
     * @brief Construct a new Tran object
     * 
     * @param host 
     */
    Tran(Host &host) : host_{host}
    {
        exitActions(host_, std::bool_constant<false>());
    }

    ~Tran()
    {
        Tran<TTarget, TSource, TTarget>::entryActions(host_,
                                                      std::bool_constant<false>());
        TTarget::initial(host_);
    }

    Host &host_;
};

/**
 * @brief Initializer for Compound States
 *
 * @tparam TTarget default target
 */
template <typename TTarget>
struct Initial
{
    using Host = typename TTarget::Host;

    Initial(Host &host) : host_{host} {}

    ~Initial()
    {
        TTarget::entry(host_);
        TTarget::initial(host_);
    }

    Host &host_;
};

template <typename T>
class Hsm
{
public:
    const HsmState<T> *state_;

    void next(const HsmState<T> &state) { state_ = &state; }
};

} // namespace hsm
#endif // HSM_HPP
