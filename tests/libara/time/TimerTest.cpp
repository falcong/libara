/*
 * $FU-Copyright$
 */

#include "CppUTest/TestHarness.h"
#include "testAPI/mocks/time/TimerMock.h"
#include "testAPI/mocks/time/TimeoutEventListenerMock.h"

using namespace ARA;

TEST_GROUP(TimerTest) {};

TEST(TimerTest, addTimeoutListener) {
    TimerMock timer;
    TimeoutEventListenerMock listener1;
    TimeoutEventListenerMock listener2;
    TimeoutEventListenerMock listener3;

    timer.addTimeoutListener(&listener1);
    timer.addTimeoutListener(&listener2);
    timer.addTimeoutListener(&listener3);

    CHECK_FALSE(listener1.hasBeenNotified());
    CHECK_FALSE(listener2.hasBeenNotified());
    CHECK_FALSE(listener3.hasBeenNotified());

    timer.expire();

    CHECK_TRUE(listener1->hasBeenNotified());
    CHECK_TRUE(listener2->hasBeenNotified());
    CHECK_TRUE(listener3->hasBeenNotified());
}

TEST(TimerTest, defaultType) {
    TimerMock timer;
    BYTES_EQUAL(5, timer.getType());
}

TEST(TimerTest, defaultContextObject) {
    TimerMock timer;
    CHECK(timer.getContextObject() == nullptr);
}

TEST(TimerTest, getType) {
    TimerMock timer = TimerMock(TimerType::ROUTE_DISCOVERY_DELAY_TIMER);
    BYTES_EQUAL(TimerType::ROUTE_DISCOVERY_DELAY_TIMER, timer.getType());
}

TEST(TimerTest, getContextObject) {
    const char* contextObject = "Hello World";
    TimerMock timer = TimerMock(TimerType::PANTS_TIMER, (void*) contextObject);
    CHECK_EQUAL(contextObject, timer.getContextObject());
}

TEST(TimerTest, setContextObject) {
    TimerMock timer = TimerMock();
    CHECK(timer.getContextObject() == nullptr);

    const char* contextObject = "Hello World";
    timer.setContextObject((void*)contextObject);
    CHECK_EQUAL(contextObject, timer.getContextObject());
}
