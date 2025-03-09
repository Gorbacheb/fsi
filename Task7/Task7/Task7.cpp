#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <numeric>

using namespace std;

enum class EventType { COMPLETION, ARRIVAL  };

struct Request {
    int id;
    int timestamp;
    string type;
    int address;
    int size;
    int start_time = -1;
    int end_time = -1;
};

struct Event {
    int time;
    EventType event_type;
    Request* req;
    bool operator>(const Event& other) const {
        if (time == other.time) {
            //if the times match, i assume that first the old request will be executed, and then the new one will be taken over
            return event_type > other.event_type;
        }
        return time > other.time;
    }
};

bool isRangesOverlap(const Request& a, const Request& b) {
    int a_start = a.address, a_end = a_start + a.size - 1; // [start, end)
    int b_start = b.address, b_end = b_start + b.size - 1;
    return (a_start <= b_end) && (b_start <= a_end);
}

void simulateServer(vector<Request>& requests, int N) {

    priority_queue<Event, vector<Event>, greater<Event>> event_queue;
    vector<Request*> active_requests, pending_queue, completed_requests;

    sort(requests.begin(), requests.end(), [](const Request& a, const Request& b) {
        return a.timestamp < b.timestamp;
    });

    for (auto& req : requests) {
        event_queue.push({ req.timestamp, EventType::ARRIVAL, &req });
    }

    while (!event_queue.empty()) {
        Event event = event_queue.top();
        event_queue.pop();

        if (event.event_type == EventType::ARRIVAL) {
            bool can_start = true;
            for (const auto& active : active_requests) {
                if (active->type == "WRITE" && isRangesOverlap(*active, *event.req)) {
                    can_start = false;
                    break;
                }
            }
            if (active_requests.size() >= N) 
                can_start = false;

            if (can_start) {
                int latency = event.req->size * (event.req->type == "WRITE" ? 1 : 2);
                int completion_time = event.time + latency;
                event_queue.push({ completion_time, EventType::COMPLETION, event.req });
                active_requests.push_back(event.req);
                event.req->start_time = event.time;
            }
            else {
                pending_queue.push_back(event.req);
            }
        }
        else if (event.event_type == EventType::COMPLETION) {
            active_requests.erase(remove(active_requests.begin(), active_requests.end(), event.req), active_requests.end());
            event.req->end_time = event.time;
            completed_requests.push_back(event.req);

            while (active_requests.size() < N && !pending_queue.empty()) {
                Request* next_req = pending_queue.front();
                bool overlap = false;
                for (const auto& active : active_requests) {
                    if (active->type == "WRITE" && isRangesOverlap(*active, *next_req)) {
                        overlap = true;
                        break;
                    }
                }
                if (!overlap) {
                    pending_queue.erase(pending_queue.begin());
                    int latency = next_req->size * (next_req->type == "WRITE" ? 1 : 2);
                    int completion_time = event.time + latency;
                    event_queue.push({ completion_time, EventType::COMPLETION, next_req });
                    active_requests.push_back(next_req);
                    next_req->start_time = event.time;
                }
                else {
                    break;
                }
            }
        }
    }

    vector<int> reads, writes;
    for (const auto& req : completed_requests) {
        int total_latency = req->end_time - req->timestamp;
        if (req->type == "READ") 
            reads.push_back(total_latency);
        else writes.push_back(total_latency);
    }

    auto compute_stats = [](vector<int>& data) {
        if (data.empty()) return;
        sort(data.begin(), data.end());
        int n = data.size();
        double avg = accumulate(data.begin(), data.end(), 0.0) / n;
        int min_val = data.front();
        int max_val = data.back();
        double median = n % 2 ? data[n / 2] : (data[n / 2 - 1] + data[n / 2]) / 2.0;

        cout << "Average: " << avg << " usec\n";
        cout << "Median: " << median << " usec\n";
        cout << "Min: " << min_val << " usec\n";
        cout << "Max: " << max_val << " usec\n";
    };

    cout << "READ statistics:\n";
    compute_stats(reads);
    cout << "\nWRITE statistics:\n";
    compute_stats(writes);
}

int main() {
    ifstream infile("input.txt");
    vector<Request> requests;
    if (!infile) {
        cerr << "Error opening input.txt" << endl;
        return 1;
    }

    int id, timestamp, address, size;
    string type;
    while (infile >> id >> timestamp >> type >> address >> size) {
        requests.push_back({ id, timestamp, type, address, size });
    }

    vector<int> test_n = { 1, 5, 10 };
    for (int n : test_n) {
        cout << "\nResults for N=" << n << ":\n";
        vector<Request> copy_requests = requests;
        simulateServer(copy_requests, n);
        cout << "------------------------\n";
    }
    return 0;
}