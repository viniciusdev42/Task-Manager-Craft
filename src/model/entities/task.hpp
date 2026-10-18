#ifndef TASK_HEADER
#define TASK_HEADER

#include <ctime>
#include <string>

enum class Priority { Low, Medium, High,Urgent };
enum class Status { Pending,InProgress, Completed, Cancelled };

std::string forString(Priority p);
std::string forString(Status s);
void printDate(std::time_t t);

struct Task {
    int id;
    std::string title;
    std::string description;
    Priority priority;
    std::time_t deadline;
    Status status;
    std::time_t timeStamp;

    Task(int id, std::string title, std::string description,
           Priority priority, std::time_t deadline);

    bool isLate() const;
    void show() const;
};

#endif // TASK_HEADER
