#include "Job.h"
#include "Queue.h"
#include <thread>
#include <string>
#include <chrono>

void workerMain(int worker_id,
                Queue<Job>& jobQ,
                Queue<Result>& resultQ)
{
  Job job;
  while (jobQ.pop(job)) {
    // mock CPU work
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    // Create Results
    Result r;
    r.job_id = job.id;
    r.message = "Hello from worker " + std::to_string(worker_id)
              + " | processed job " + std::to_string(job.id)
              + " | payload = [" + job.payload + "]";

    // move to Result
    resultQ.push(std::move(r));
  }
  // jobQ.stop() called -> threads are over
}

// Example Code
// ------------------------------
// 4) main(): (OpenGL에선 이 위치가 "Render thread" 역할)
//    - job을 enqueue
//    - 결과를 dequeue 해서 출력
//    - 종료 처리(stop + join)
// ------------------------------
// int main() {
//   BlockingQueue<Job> jobQ;
//   BlockingQueue<Result> resultQ;

//   const int NUM_WORKERS = 5;
//   const int NUM_JOBS    = 12;

//   // 4-1) 워커 스레드 5개 시작
//   std::vector<std::thread> workers;
//   workers.reserve(NUM_WORKERS);
//   for (int i = 0; i < NUM_WORKERS; ++i) {
//     workers.emplace_back(workerMain, i, std::ref(jobQ), std::ref(resultQ));
//   }

//   // 4-2) 메인 스레드가 Job을 계속 enqueue (일감을 던진다)
//   for (int j = 0; j < NUM_JOBS; ++j) {
//     Job job;
//     job.id = j;
//     job.payload = "work item #" + std::to_string(j);
//     jobQ.push(std::move(job));
//   }

//   // 4-3) 결과를 NUM_JOBS개 받을 때까지 꺼내서 출력
//   //      (OpenGL에선 여기서 "결과를 받아 VBO 업로드 + draw"로 바뀜)
//   for (int received = 0; received < NUM_JOBS; ++received) {
//     Result r;
//     // resultQ.pop()은 결과가 없으면 잠듦 (여기서는 괜찮음)
//     resultQ.pop(r);
//     std::cout << "[MAIN] got result: " << r.message << "\n";
//   }

//   // 4-4) 종료 처리:
//   //      더 이상 job을 만들지 않으니 stop()으로 워커들을 깨워서 종료시킴
//   jobQ.stop();

//   // 워커 스레드가 끝날 때까지 기다림
//   for (auto& t : workers) t.join();

//   std::cout << "All workers joined. Done.\n";
//   return 0;
// }

// bool try_pop(T& out) {
//   std::lock_guard<std::mutex> lk(m_);
//   if (q_.empty()) return false;
//   out = std::move(q_.front());
//   q_.pop();
//   return true;
// }
