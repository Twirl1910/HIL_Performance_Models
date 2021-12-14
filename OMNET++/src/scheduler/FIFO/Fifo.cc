#include "Fifo.h"
#include <algorithm>
#include <vector>
#include "../../stats/stats.h"

Define_Module(Fifo);

void Fifo::initialize() {
    Scheduler::initialize();

    for (int i = min_priority; i <= max_priority; i++) {
        std::deque<Task*>* q = new std::deque<Task*>();
        this->queues.push_back(*q);
    }
}

void Fifo::add(Task* t) {
    queues[t->getPriority() - min_priority].push_back(t);
    Stats::recordTaskAdd(getParentModule()->getFullName(), t, simTime());
    EV_INFO << "SCHEDULE TASK FROM: " << t->getSenderModule()->getFullName() << endl;
}

/**
 * FIFO scheduler implementer after linux SCHED_FIFO strategy
 * See `man 7 sched` for reference
 *
 * returns a negative time when there is nothing to schedule / queues are empty.
 */
simtime_t Fifo::schedule() {
    simtime_t diff = simTime() - lastTime;
    lastTime = simTime();

    // update time and collect completed tasks
    for (int i = 0; i < cores; i++) {
        if (cpu[i]->getState() == State::BUSY) {
            Task* t = cpu[i]->getCurrent();
            t->setTime(t->getTime() - diff.dbl());

            if (t->getTime() <= 0) {
                cpu[i]->release();
                Stats::recordTaskCompleted(getParentModule()->getFullName(), t, simTime());
                EV_INFO << "NOW COMPLETED: " << t->getSenderModule()->getFullName() << endl;
                sendDirect(t, t->getSenderModule(), "scheduler");
            }
        }
    }

    // reschedule
    for (int p = max_priority - min_priority; p >= 0; p--) {
        if (queues[p].empty()) continue;

        for (int i = 0; i < cores; i++) {
            if (queues[p].empty()) break;

            // assign directly if cpu is free
            if (cpu[i]->getState() == State::IDLE) {
                Task* t = queues[p].front();
                queues[p].pop_front();
                cpu[i]->assign(t);
                Stats::recordTaskRun(getParentModule()->getFullName(), t, simTime());
            }
            // replace with higher priority thread. lower prio thread inserted at front of its prio queue
            else if (cpu[i]->getState() == State::BUSY && p + min_priority > cpu[i]->getCurrent()->getPriority()) {
                Task* ta = cpu[i]->getCurrent();
                Task* tn = queues[p].front();
                queues[p].pop_front();
                cpu[i]->release();
                queues[p].push_front(ta);
                cpu[i]->assign(tn);
                Stats::recordTaskRun(getParentModule()->getFullName(), tn, simTime());
            }
        }
    }

    // get remaining time on each core
    std::vector<double> remaining;
    for (int i = 0; i < cores; i++) {
        if (cpu[i]->getState() == State::BUSY) {
            EV_INFO << "NOW RUNNING " << cpu[i]->getCurrent()->getSenderModule()->getFullName() << " ON CORE " << i << " for total " << cpu[i]->getCurrent()->getTime() << endl;
            remaining.push_back(cpu[i]->getCurrent()->getTime());
        }
    }

    // return
    if (!remaining.empty()) {
        double next = *std::min_element(remaining.begin(), remaining.end()); // time until the next task is completed
        return lastTime + next;
    }
    else {
        return SimTime(-1, SIMTIME_S);
    }
}
