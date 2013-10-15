// linux: g++ task_mgr.cpp -o task_mgr -std=c++11 -lpthread

#include <iostream>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <chrono>

class Task
{
public:
  virtual ~Task(void) {}
  virtual std::string GetName(void) const = 0;
  virtual void Run(void) = 0;
};

class Task0 : public Task
{
public:
  virtual std::string GetName(void) const { return "task0"; }
  virtual void Run(void) { std::cout << "Running task0" << std::endl; std::this_thread::sleep_for(std::chrono::milliseconds(400)); std::cout << "done" << std::endl; }
};

class Task1 : public Task
{
public:
  virtual std::string GetName(void) const { return "task1"; }
  virtual void Run(void) { std::cout << "Running task1" << std::endl; std::this_thread::sleep_for(std::chrono::milliseconds(100)); std::cout << "done" << std::endl; }
};

class Task2 : public Task
{
public:
  virtual std::string GetName(void) const { return "task2"; }
  virtual void Run(void) { std::cout << "Running task2" << std::endl; std::this_thread::sleep_for(std::chrono::milliseconds(200)); std::cout << "done" << std::endl; }
};

class TaskManager
{
public:
  TaskManager(void)
    : m_Running(false)
  {
    Start();
  }

  ~TaskManager(void)
  {
    if (m_Running)
      Stop();
  }

  void Start(void)
  {
    if (m_Running)
      return;

    m_Running = true;
    m_Thread = std::thread([&]()
    {
      while (m_Running)
      {
        std::unique_lock<std::mutex> Lock(m_Mutex);

        while (m_Tasks.empty())
          m_CondVar.wait(Lock);

        auto pCurTask = m_Tasks.front();
        m_Tasks.pop();
        if (pCurTask == nullptr)
        {
          m_Running = false;
          break;
        }
        pCurTask->Run();
        delete pCurTask;
      }

      while (!m_Tasks.empty())
      {
        auto pCurTask = m_Tasks.front();
        m_Tasks.pop();
        if (pCurTask)
        {
          pCurTask->Run();
          delete pCurTask;
        }
      }
    });
  }

  void Stop(void)
  {
    if (!m_Running)
      return;

    m_Running = false;

    m_Mutex.lock();
    m_Tasks.push(nullptr);
    m_CondVar.notify_one();
    m_Mutex.unlock();
    m_Thread.join();
  }

  void AddTask(Task* pTask)
  {
    if (!m_Running)
    {
      delete pTask;
      return;
    }

    if (pTask == nullptr)
      return;

    std::unique_lock<std::mutex> Lock(m_Mutex);
    m_Tasks.push(pTask);
    m_CondVar.notify_one();
  }

private:
  std::atomic<bool>       m_Running;
  std::thread             m_Thread;
  std::condition_variable m_CondVar;
  std::queue<Task*>       m_Tasks;
  std::mutex              m_Mutex;
};

int main(void)
{
  TaskManager TaskMgr;

  TaskMgr.AddTask(new Task0);
  TaskMgr.AddTask(new Task1);
  TaskMgr.AddTask(new Task2);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  TaskMgr.AddTask(new Task1);
  std::cout << "Reach end of main" << std::endl;
}
