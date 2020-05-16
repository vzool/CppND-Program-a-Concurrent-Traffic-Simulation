#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_messages.back());
    _messages.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T&& msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    _messages.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        if(_messages.receive() == TrafficLightPhase::green)
        {
            return;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // wait 1ms between two cycles
    };
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method "cycleThroughPhases" should be started in a thread when the public method "simulate" is called.
    // To do this, use the thread queue in the base class.

    std::lock_guard<std::mutex> _(_mutex);
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    auto start = std::chrono::steady_clock::now();

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(4, 6);

    auto dice_rand = dis(gen);

    while(true)
    {
        auto end = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        if(diff > std::min(3, dice_rand) && diff < std::max(7, dice_rand))
        {
            if(_currentPhase == TrafficLightPhase::green)
            {
                _currentPhase = TrafficLightPhase::red;
                _messages.send(std::move(TrafficLightPhase::red));
            }
            else
            {
                _currentPhase = TrafficLightPhase::green;
                _messages.send(std::move(TrafficLightPhase::green));
            }

            start = std::chrono::steady_clock::now();
            dice_rand = dis(gen);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // wait 1ms between two cycles
    };
}
