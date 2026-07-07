// MutliThreading.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <conio.h>
#include <string>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>

using namespace std;

#define REPS 500000

/*
    입력, 처리, 출력 이 세개의 쓰레드를 동시에 돌려야함
*/

enum class EInputType
{
    Up,
    Down,
    Left,
    Right
};

#define USE_BLOCKING_QUEUE false

#define USE_MANAGER_BLOCKING_METHOD  (USE_BLOCKING_QUEUE)? false : false // 블로킹큐를 쓴다면 매니저 내에 정의되어있는 블록을 쓸 필요가 없음.

template<typename T>
class ThreadSafeQueue
{

public:
    void AddElem(T InJobs)
    {
        lock_guard<mutex> lck(usingMutex);
        jobQueue.push(std::move(InJobs));
    }

    bool TryGetElem(T& OutType)
    {
        lock_guard<mutex> lck(usingMutex);
        if (jobQueue.empty())
        {
            return false;
        }
        OutType = std::move(jobQueue.front());
        jobQueue.pop();

        return true;
    }

    bool IsEmpty() const
    {
        return jobQueue.empty();
    }

protected:
    queue<T> jobQueue;
    mutex usingMutex;

};

template<typename T>
class BlockingQueue
{
public:
    BlockingQueue()
    {
        bIsActivated = true;
    }
    ~BlockingQueue()
    {
    }
    void AddElem(T InJobs)
    {
        {
            unique_lock<mutex> lck(usingMutex);
            usingQueue.AddElem(InJobs);
        }
        cv.notify_one();
    }

    void GetElemWait(T& OutType)
    {
        unique_lock<mutex> lck(usingMutex);
        if (bIsActivated == false)
        {
            return;
        }

        cv.wait(lck, [this] {
            return usingQueue.IsEmpty() == false && bIsActivated;
            });

        usingQueue.TryGetElem(OutType);
    }

    void Shutdown()
    {
        bIsActivated = false;
        cv.notify_all();
    }
protected:
    ThreadSafeQueue<T> usingQueue;
    mutex usingMutex;
    condition_variable cv;
    bool bIsActivated = false;

};

mutex coutMutex;
void PrintString(const string& Str)
{
    lock_guard<mutex> lock(coutMutex);
    cout << Str << endl;
}

#if USE_BLOCKING_QUEUE
BlockingQueue<EInputType> GBlockingInputQueue;
BlockingQueue<string> GBlockingOutputQueue;
#else
ThreadSafeQueue<EInputType> GThreadSafeInputQueue;
ThreadSafeQueue<string> GThreadSafeOutputQueue;
#endif
function<void()> OnInputDetectCallback;
function<void()> OnOutputDetectCallback;

void OnInputDetected(EInputType In)
{
#if USE_BLOCKING_QUEUE

        GBlockingInputQueue.AddElem(In);
#else

        GThreadSafeInputQueue.AddElem(In);
#endif

    if (OnInputDetectCallback)
    {
        OnInputDetectCallback();
    }
}

void OnOutputDetected(string Output)
{
#if USE_BLOCKING_QUEUE

        GBlockingOutputQueue.AddElem(Output);
#else
        GThreadSafeOutputQueue.AddElem(Output);
#endif

    if (OnOutputDetectCallback)
    {
        OnOutputDetectCallback();
    }
}

int TotalWorkoutCnt; // Atomic

void PushUp500000()
{
    //이걸 따로 출력하지는
    int pushUpCnt = 0;
    for (int i = 0; i < REPS; ++i)
    {
        ++pushUpCnt;
    }

    ++TotalWorkoutCnt;

    string msg = "pushUpCnt : " + to_string(pushUpCnt) + " / " + "totalWorkoutCnt : " + to_string(TotalWorkoutCnt);
    OnOutputDetected(move(msg));
}

void PullUp500000()
{
    int pullUpCnt = 0;
    for (int i = 0; i < REPS; ++i)
    {
        ++pullUpCnt;
    }

    ++TotalWorkoutCnt;

    string msg = "pullUpCnt : " + to_string(pullUpCnt) + " / " + "totalWorkoutCnt : " + to_string(TotalWorkoutCnt);
    OnOutputDetected(move(msg));
}

void Squat500000()
{
    int squatCnt = 0;
    for (int i = 0; i < REPS; ++i)
    {
        ++squatCnt;
    }

    ++TotalWorkoutCnt;

    string msg = "squatCnt : " + to_string(squatCnt) + " / " + "totalWorkoutCnt : " + to_string(TotalWorkoutCnt);
    OnOutputDetected(move(msg));
}

void Crunch500000()
{
    int crunchCnt = 0;
    for (int i = 0; i < REPS; ++i)
    {
        ++crunchCnt;
    }

    ++TotalWorkoutCnt;

    string msg = "crunchCnt : " + to_string(crunchCnt) + " / " + "totalWorkoutCnt : " + to_string(TotalWorkoutCnt);
    OnOutputDetected(move(msg));
}

//pure virtual
class ThreadRunnable
{
public:
    ThreadRunnable()
    {
    }
    void Start()
    {
        bIsRunning = true;
        t = thread(&ThreadRunnable::Run,this);
    }

    bool IsRunning() const
    {
        return bIsRunning;
    }

    void Stop()
    {
        bIsRunning = false;
#if USE_MANAGER_BLOCKING_METHOD
        cv.notify_all();
#endif
    }

    void StopAndJoin()
    {
        Stop();
        Join();
    }

    void Join()
    {
        if (t.joinable())
            t.join();
    }

    virtual void Run() = 0;

protected:
    thread t;
    atomic<bool> bIsRunning;
#if USE_MANAGER_BLOCKING_METHOD
protected:
    condition_variable cv;
    mutex usingMutex;
#endif
};

bool bGlobalRunning = false;

class InputManager : public ThreadRunnable
{
public:
    InputManager() : ThreadRunnable()
    {

    }

    virtual void Run() override
    {
        while (bIsRunning)
        {
            char key = _getch(); // 키를 누르는 즉시 반환

            if (key == ' ')
                continue;

            switch (key)
            {
                case 'w':
                case 'W':
                    OnInputDetected(EInputType::Up);
                    break;
                case 's':
                case 'S':
                    OnInputDetected(EInputType::Down);
                    break;
                case 'a':
                case 'A':
                    OnInputDetected(EInputType::Left);
                    break;
                case 'd':
                case 'D':
                    OnInputDetected(EInputType::Right);
                    break;
                case 'b':
                case 'B':
                {
                    bGlobalRunning = false;
                }
                    break;
                default:
                    break;
            }

            const string InputKey = "입력한 키: " + string(1,key);
            PrintString(InputKey);
        }
    }
};

class ProcessManager : public ThreadRunnable
{
public:
    ProcessManager() : ThreadRunnable()
    {
        OnInputDetectCallback = [this]() {
            OnInputDetected();
            };
    }

    virtual void Run() override
    {
        while (bIsRunning)
        {
            EInputType top;
#if USE_BLOCKING_QUEUE
            GBlockingInputQueue.GetElemWait(top);
#else
        #if USE_MANAGER_BLOCKING_METHOD
            unique_lock<mutex> lck(usingMutex);
            cv.wait(lck, [this]() {
                return GThreadSafeInputQueue.IsEmpty() == false && bIsRunning;
                });

            GThreadSafeInputQueue.TryGetElem(top);
        #else
            if (GThreadSafeInputQueue.IsEmpty())
            {
                continue;
            }
            GThreadSafeInputQueue.TryGetElem(top);
        #endif
#endif

            switch (top)
            {
            case EInputType::Up:
                PushUp500000();
                break;
            case EInputType::Down:
                PullUp500000();
                break;
            case EInputType::Left:
                Squat500000();
                break;
            case EInputType::Right:
                Crunch500000();
                break;
            default:
                break;
            }

            PrintString("Doing my job!");
        }
    }

    void OnInputDetected()
    {
#if USE_MANAGER_BLOCKING_METHOD
        cv.notify_one();
#endif
    }
};

class OutputManager : public ThreadRunnable
{
public:
    OutputManager() : ThreadRunnable()
    {
        OnOutputDetectCallback = [this]() {
            OnOutputDetected();
            };
    }

    virtual void Run() override
    {
        while (bIsRunning)
        {
            string top;
#if USE_BLOCKING_QUEUE
            GBlockingOutputQueue.GetElemWait(top);
#else
            #if USE_MANAGER_BLOCKING_METHOD
                        unique_lock<mutex> lck(usingMutex);
                        cv.wait(lck, [this]() {
                            return GThreadSafeOutputQueue.IsEmpty() == false && bIsRunning;
                            });

                        GThreadSafeOutputQueue.TryGetElem(top);
            #else
                        if (GThreadSafeOutputQueue.IsEmpty())
                            continue;
                        GThreadSafeOutputQueue.TryGetElem(top);
            #endif
#endif
            PrintString(top);
        }
    }
    
    void OnOutputDetected()
    {
#if USE_MANAGER_BLOCKING_METHOD
        cv.notify_one();
#endif
    }

#if USE_MANAGER_BLOCKING_METHOD
protected:
    condition_variable cv;
    mutex usingMutex;
#endif
};


class Application
{
public:
    Application()
    {
        IM = make_shared<InputManager>();
        PM = make_shared<ProcessManager>();
        OM = make_shared<OutputManager>();

        IM->Start();
        PM->Start();
        OM->Start();
    }

    bool IsRunning()
    {
        return IM->IsRunning();
    }

    void Shutdown()
    {
#if USE_BLOCKING_QUEUE
        GBlockingInputQueue.Shutdown();
        GBlockingOutputQueue.Shutdown();
#endif
        IM->Stop();
        PM->Stop();
        OM->Stop();

        IM->Join();
        PM->Join();
        OM->Join();
    }
protected:
    shared_ptr<InputManager> IM;
    shared_ptr<ProcessManager> PM;
    shared_ptr<OutputManager> OM;
};


int main()
{
    Application app;
    bGlobalRunning = true;

    while (bGlobalRunning)
    {

    }

    app.Shutdown();


    std::cout << "Hello World!\n";
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
