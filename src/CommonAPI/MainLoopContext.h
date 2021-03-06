/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_MAIN_LOOP_CONTEXT_H_
#define COMMONAPI_MAIN_LOOP_CONTEXT_H_


#include <stdint.h>

#ifdef WIN32
#include <WinSock2.h>
#else
#include <poll.h>
#endif

#ifdef WIN32
#undef max
#endif

#include <limits>
#include <vector>
#include <chrono>
#include <list>
#include <functional>

namespace CommonAPI {

enum class DispatchPriority {
    VERY_HIGH,
    HIGH,
    DEFAULT,
    LOW,
    VERY_LOW
};


int64_t getCurrentTimeInMs();


/**
 * \brief Describes a basic element that periodically needs to be dispatched.
 *
 * A DispatchSource is not directly related to a file descriptor, but
 * may be dependent on a watch that manages a file descriptor. If this
 * is the case, the corresponding Watch will provide information about
 * which DispatchSources are dependent.
 */
struct DispatchSource {
    virtual ~DispatchSource() {}

    /**
     * Indicates whether this source is ready to be dispatched.
     * "Prepare" will be called before polling the file descriptors.
     *
     * @return 'true' if the source is ready to be dispatched.
     */
    virtual bool prepare(int64_t& timeout) = 0;

    /**
     * Indicates whether this source is ready to be dispatched.
     * "Check" will be called after polling the file descriptors.
     *
     * @return 'true' if the source is ready to be dispatched.
     */
    virtual bool check() = 0;

    /**
     * The return value indicates whether this dispatch source currently has
     * more data to dispatch. The mainloop may chose to ignore the return value.
     *
     * @return 'true' if there currently is more to dispatch, 'false' if not.
     */
    virtual bool dispatch() = 0;
};


/**
 * \brief Describes an element that manages a file descriptor.
 *
 * The watch is ready to be dispatched whenever it's managed file descriptor
 * has events in it's revents-field.
 *
 * It is possible that there are DispatchSources of which the dispatch readiness
 * directly depends on the dispatching of the watch. If this is the case, such
 * DispatchSources can be retrieved from this Watch.
 */
struct Watch {
    virtual ~Watch() {}

    /**
     * \brief Dispatches the watch.
     *
     * Should only be called once the associated file descriptor has events ready.
     *
     * @param eventFlags The events that shall be retrieved from the file descriptor.
     */
    virtual void dispatch(unsigned int eventFlags) = 0;

    /**
     * \brief Returns the file descriptor that is managed by this watch.
     *
     * @return The associated file descriptor.
     */
    virtual const pollfd& getAssociatedFileDescriptor() = 0;

    /**
     * \brief Returns a vector of all dispatch sources that depend on the watched file descriptor.
     *
     * The returned vector will not be empty if and only if there are any sources
     * that depend on availability of data of the watched file descriptor. Whenever this
     * Watch is dispatched, those sources likely also need to be dispatched.
     */
    virtual const std::vector<DispatchSource*>& getDependentDispatchSources() = 0;
};

const int64_t TIMEOUT_INFINITE = std::numeric_limits<int64_t>::max();
const int64_t TIMEOUT_NONE = 0;


/**
 * \brief Describes a basic timeout.
 *
 * Timeouts will be taken into consideration when waiting in a call to poll
 * for a file descriptor to become ready. When the lowest known timeout expires,
 * the call to poll will return, regardless of whether a file descriptor was ready
 * or not.
 */
struct Timeout {
    virtual ~Timeout() {}

    /**
     * Needs to be called when this timeout is expired.
     *
     * @return 'true' if the timeout shall be rescheduled, 'false' if it shall be removed.
     */
    virtual bool dispatch() = 0;

    /**
     * \brief The timeout interval in milliseconds.
     *
     * Returns TIMEOUT_INFINITE for "dispatch never", TIMEOUT_NONE for "dispatch immediately",
     * or any positive value as an interval of time in milliseconds that needs to pass before
     * this timeout is to be dispatched.
     */
    virtual int64_t getTimeoutInterval() const = 0;

    /**
     * \brief Returns the point in time at which this timeout needs to be dispatched next.
     *
     * After a initialization and after each dispatch, this timeout will re-calculate it's next
     * ready time. This value may be ignored if a different mechanism for monitoring timeout intervals
     * is used.
     */
    virtual int64_t getReadyTime() const = 0;
};


typedef std::function<void(DispatchSource*, const DispatchPriority)> DispatchSourceAddedCallback;
typedef std::function<void(DispatchSource*)> DispatchSourceRemovedCallback;
typedef std::function<void(Watch*, const DispatchPriority)> WatchAddedCallback;
typedef std::function<void(Watch*)> WatchRemovedCallback;
typedef std::function<void(Timeout*, const DispatchPriority)> TimeoutSourceAddedCallback;
typedef std::function<void(Timeout*)> TimeoutSourceRemovedCallback;
typedef std::function<void()> WakeupCallback;

typedef std::list<std::pair<DispatchSourceAddedCallback, DispatchSourceRemovedCallback>> DispatchSourceListenerList;
typedef std::list<std::pair<WatchAddedCallback, WatchRemovedCallback>> WatchListenerList;
typedef std::list<std::pair<TimeoutSourceAddedCallback, TimeoutSourceRemovedCallback>> TimeoutSourceListenerList;
typedef std::list<WakeupCallback> WakeupListenerList;

typedef DispatchSourceListenerList::iterator DispatchSourceListenerSubscription;
typedef WatchListenerList::iterator WatchListenerSubscription;
typedef TimeoutSourceListenerList::iterator TimeoutSourceListenerSubscription;
typedef WakeupListenerList::iterator WakeupListenerSubscription;


/**
 * \brief Provides hooks for your Main Loop implementation.
 *
 * By registering callbacks with this class, you will be notified about all DispatchSources,
 * Watches, Timeouts and Wakeup-Events that need to be handled by your Main Loop implementation.
 *
 */
class MainLoopContext {
 public:
    MainLoopContext() {}
    MainLoopContext(const MainLoopContext&) = delete;
    MainLoopContext& operator=(const MainLoopContext&) = delete;
    MainLoopContext(MainLoopContext&&) = delete;
    MainLoopContext& operator=(MainLoopContext&&) = delete;

    /**
     * \brief Registers for all DispatchSources that are added or removed.
     */
    DispatchSourceListenerSubscription subscribeForDispatchSources(DispatchSourceAddedCallback dispatchAddedCallback, DispatchSourceRemovedCallback dispatchRemovedCallback);

    /**
     * \brief Registers for all Watches that are added or removed.
     */
    WatchListenerSubscription subscribeForWatches(WatchAddedCallback watchAddedCallback, WatchRemovedCallback watchRemovedCallback);

    /**
     * \brief Registers for all Timeouts that are added or removed.
     */
    TimeoutSourceListenerSubscription subscribeForTimeouts(TimeoutSourceAddedCallback timeoutAddedCallback, TimeoutSourceRemovedCallback timeoutRemovedCallback);

    /**
     * \brief Registers for all Wakeup-Events that need to interrupt a call to "poll".
     */
    WakeupListenerSubscription subscribeForWakeupEvents(WakeupCallback wakeupCallback);

    /**
     * \brief Unsubscribes your listeners for DispatchSources.
     */
    void unsubscribeForDispatchSources(DispatchSourceListenerSubscription subscription);

    /**
     * \brief Unsubscribes your listeners for Watches.
     */
    void unsubscribeForWatches(WatchListenerSubscription subscription);

    /**
     * \brief Unsubscribes your listeners for Timeouts.
     */
    void unsubscribeForTimeouts(TimeoutSourceListenerSubscription subscription);

    /**
     * \brief Unsubscribes your listeners for Wakeup-Events.
     */
    void unsubscribeForWakeupEvents(WakeupListenerSubscription subscription);

    /**
     * \brief Notifies all listeners about a new DispatchSource.
     */
    void registerDispatchSource(DispatchSource* dispatchSource, const DispatchPriority dispatchPriority = DispatchPriority::DEFAULT);

    /**
     * \brief Notifies all listeners about the removal of a DispatchSource.
     */
    void deregisterDispatchSource(DispatchSource* dispatchSource);

    /**
     * \brief Notifies all listeners about a new Watch.
     */
    void registerWatch(Watch* watch, const DispatchPriority dispatchPriority = DispatchPriority::DEFAULT);

    /**
     * \brief Notifies all listeners about the removal of a Watch.
     */
    void deregisterWatch(Watch* watch);

    /**
     * \brief Notifies all listeners about a new Timeout.
     */
    void registerTimeoutSource(Timeout* timeoutEvent, const DispatchPriority dispatchPriority = DispatchPriority::DEFAULT);

    /**
     * \brief Notifies all listeners about the removal of a Timeout.
     */
    void deregisterTimeoutSource(Timeout* timeoutEvent);

    /**
     * \brief Notifies all listeners about a wakeup event that just happened.
     */
    void wakeup();

    /**
     * \brief Will return true if at least one subscribe for DispatchSources or Watches has been called.
     *
     * This function will be used to prevent creation of a factory if a mainloop context is given, but
     * no listeners have been registered. This is done in order to ensure correct use of the mainloop context.
     */
    bool isInitialized();

 private:
    DispatchSourceListenerList dispatchSourceListeners_;
    WatchListenerList watchListeners_;
    TimeoutSourceListenerList timeoutSourceListeners_;
    WakeupListenerList wakeupListeners_;
};


} // namespace CommonAPI


#endif /* MAIN_LOOP_CONTEXT_H_ */
