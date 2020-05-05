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
     * @tparam TCurrent 
     * @param host 
     * @param x 
     */
    template <typename TCurrent>
    void handle(THost &host, const TCurrent &x) const
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
     * @tparam TCurrent 
     * @param host 
     * @param x 
     */
    template <typename TCurrent>
    void handle(THost &, const TCurrent &) const {}

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
     * @tparam TCurrent 
     * @param host 
     * @param x 
     */
    template <typename TCurrent>
    void handle(THost &host, const TCurrent &x) const
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
    static void initial(THost &host)
    {
        host.state_ = &obj_;
    }

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
        eCB_S = std::is_base_of<CurrentBase, TSource>(),
        eCB_T = std::is_base_of<CurrentBase, TTarget>(),
        eT_S = std::is_base_of<TTarget, TSource>(),
        e_CB_EQ_S = std::is_same<CurrentBase, TSource>(),
        e_S_EQ_T = std::is_same<TSource, TTarget>(),

        exit_stop = eCB_T && eCB_S && !(e_S_EQ_T && e_CB_EQ_S),
        entry_stop_initial = eT_S && !e_S_EQ_T,
        entry_stop = eCB_S,
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

    Tran(Host &host) : host_{host}
    {
        exitActions(host_, std::bool_constant<false>());
    }

    ~Tran()
    {
        Tran<TTarget, TSource, TTarget>::entryActions(
            host_, std::bool_constant<entry_stop_initial>());
        TTarget::initial(host_);
    }

    Host &host_;
};

/**
 * @brief   Initial transition object
 *
 * @tparam TCurrent
 * @tparam TSource
 * @tparam TTarget
 */
template <typename TCurrent, typename TSource, typename TTarget>
struct InitialTran
{
    using Host = typename TCurrent::Host;
    using CurrentBase = typename TCurrent::Base;
    using SourceBase = typename TSource::Base;
    using TargetBase = typename TTarget::Base;

    enum
    {
        // work out when to terminate template recursion
        e_CB_EQ_S = std::is_same<CurrentBase, TSource>(),

        entry_stop = e_CB_EQ_S,
    };

    static void entryActions(Host &, std::true_type) {}

    static void entryActions(Host &host, std::false_type)
    {
        InitialTran<CurrentBase, TSource, TTarget>::entryActions(
            host, std::bool_constant<entry_stop>());
        TCurrent::entry(host);
    }

    InitialTran(Host &host) : host_{host}
    {
    }

    ~InitialTran()
    {
        InitialTran<TTarget, TSource, TTarget>::entryActions(
            host_, std::bool_constant<false>());
        TTarget::initial(host_);
    }

    Host &host_;
};

template <typename T>
class Hsm
{
public:
    /*
     * Current leaf state 
     */
    const HsmState<T> *state_;
};

/**
 * @brief Macro for defining initial state transition
 * 
 */
#define HSM_INITIAL(HOST, STATE, INITSTATE)      \
    template <>                                  \
    inline void STATE::initial(HOST &h)          \
    {                                            \
        InitialTran<This, This, INITSTATE> t(h); \
    }

} // namespace hsm
#endif // HSM_HPP
