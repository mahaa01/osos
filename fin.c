#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX 10 //process max num
#define QUANTUM_TIME 3 //rr 에서 쓰이는

typedef struct process* processPointer;

typedef struct process {
    int process_num;
    int PID;
    int arrival;
    int CPU_burst;
    int IO_burst;
    int priority;
    int CPUremain_t;
    int IOremain_t;
    int waiting_t;
    int turnaround_t;
} process;

process orig[MAX];
process cop[MAX];// copy 본

typedef struct evaluation* evaPointer;
typedef struct evaluation {
    int alg;
    int startTime;
    int endTime;
    int avg_waiting_t;
    int avg_turnaround_t;
} evaluation;

evaPointer evals[MAX];
int now_eval_num = 0;

int process_num;
int comput_idle = 0;
int comput_start = 0;
int comput_end = 0;
int consumtime = 0;

processPointer jobQueue[MAX];
int now_proc_num_jQ = 0;

processPointer readyQueue[MAX];
int now_proc_num_rQ = 0;

processPointer waitingQueue[MAX];
int now_proc_num_wQ = 0;

processPointer termin[MAX]; //terminate 된 process
int now_proc_num_ter = 0;

processPointer runningproc = NULL; //running 중인 proc

char above_chart[1000] = {0}; //Gant chart 윗줄 초기화
char down_chart[1000] = {0};

void init_evals() {     //evaluation 초기화
    now_eval_num = 0;
    for (int i = 0; i < MAX; i++) {
        evals[i] = NULL;
    }
}

void clear_evals() {  //메모리 수거 
    for (int i = 0; i < MAX ; i++) {
        if (evals[i] != NULL) {
            free(evals[i]);
            evals[i] = NULL;
        }
    }
    now_eval_num = 0;
}

void init_jQ() { //job queue 초기화 
    now_proc_num_jQ = 0;
    for (int i = 0; i < MAX; i++) {
        jobQueue[i] = NULL;
    }
}

void clear_jQ() {   //job queue 메모리 회수 
    for (int i = 0; i < MAX; i++) {
        jobQueue[i] = NULL;
    }
    now_proc_num_jQ = 0;
}

//pid 중복되는지 검사 ->create process 할때 pid 다 다르게 생성 위해 

int is_PID_duplicate(int new_PID, int count) {
    for (int i = 0; i < count; i++) {
        if (orig[i].PID == new_PID) {
            return 1; //중복되는 경우 
        }
    }
    return 0;//중복 되지 않는 경우 
}

void copy_process() {// 프로세스 정보 복사(ori -> p)
    for (int i = 0; i < process_num; i++) {
        cop[i] = orig[i];
    }
}

//job queue 에 insert
int insert_jQ(processPointer proc) { 
    if (now_proc_num_jQ < MAX) { //현재의 job queue num이 MAX 보다 작을때 
        jobQueue[now_proc_num_jQ++] = proc; //jobqueue 의 새로운 공간에 process 넣어주기 
        return 1;
    }
    return 0;//queue full
}

void sort_jQ() { //job queue 정렬
    int i, j;
    processPointer temp;
    for (i = 1; i < now_proc_num_jQ; i++) {
        temp = jobQueue[i];
        for (j = i - 1; j >= 0 && jobQueue[j]->arrival > temp->arrival; j--) {
            jobQueue[j + 1] = jobQueue[j]; //job queue 를 도착시간 기준으로 정렬 
            //도착 시간이 더 크면 뒤로 정렬 
        }
        jobQueue[j + 1] = temp; //앞에 있던 것은 뒤로 
    }
}

processPointer remove_jQ(processPointer proc) { //job queue 에서 빼내오기 
    for (int i = 0; i < now_proc_num_jQ; i++) {
        if (jobQueue[i] == proc) {
            processPointer removed_proc = jobQueue[i]; //i 번째 지우기 
            for (int j = i; j < now_proc_num_jQ - 1; j++) { //i 에서 끝-1 까지(하나 지워서 끝-1 해줘야함 )
                jobQueue[j] = jobQueue[j + 1]; //한칸씩 앞으로 
            }
            jobQueue[--now_proc_num_jQ] = NULL; //마지막 배열 null 로 한 후에 후위 감소 
            return removed_proc; 
        }
    }
    return NULL;//queue empty
}

void remove_duplicate_pids_in_jQ() {//job queue 에서 중복되는 pid 지우기 
    for (int i = 0; i < now_proc_num_jQ - 1; i++) {
        for (int j = i + 1; j < now_proc_num_jQ; j++) {
            if (jobQueue[i] != NULL && jobQueue[j] != NULL && jobQueue[i]->PID == jobQueue[j]->PID) { //jobqueue 에서 중복되면 
                for (int k = j; k < now_proc_num_jQ - 1; k++) {//같은 두 수 존재 같은 pid
                    jobQueue[k] = jobQueue[k + 1]; //k 없애 버리기 
                }
                jobQueue[--now_proc_num_jQ] = NULL; //마지막 배열 null 로 그리고 총 num-1
                j--;
            }
        }
    }
}

//ready queue init
void init_rQ() {
    now_proc_num_rQ = 0;
    for (int i = 0; i < MAX; i++) {
        readyQueue[i] = NULL;
    }
}

//job queue 에서 pid 가저오기 

int getProcByPid_jQ(int givenPid) {
    int result = -1;
    for (int i = 0; i < now_proc_num_jQ; i++) {
        int temp = jobQueue[i]->PID;
        if (temp == givenPid)
            return i;
    }
    return result; //못가져옴
}


//ready queue 에서 pid 가져오기 
int getProcByPid_rQ(int givenPid) {
    int result = -1;
    for (int i = 0; i < now_proc_num_rQ; i++) {
        int temp = readyQueue[i]->PID;
        if (temp == givenPid)
            return i;
    }
    return result;
}

//ready queue 초기화 
void clear_rQ() {
    for (int i = 0; i < MAX; i++) {
        readyQueue[i] = NULL;
    }
    now_proc_num_rQ = 0;
}

//ready queue에 proc insert 하기 
void insert_rQ(processPointer proc) {
    if (now_proc_num_rQ < MAX) {
        int temp = getProcByPid_rQ(proc->PID); //proc 의 pid 를 가져오기 
        if (temp != -1) { //temp 값이 있으면 즉 ready queue 에 넣으려고 하는 proc 의 pid 가 이미 존재하면
            printf("<ERROR> The process with pid: %d already exists in Ready Queue\n", proc->PID);
            return;
        }
        readyQueue[now_proc_num_rQ++] = proc; //ready queue 빈 마지막 배열에 새로운 proc 추가 (ready queue 에 proc 없다면 )
    } else {
        puts("<ERROR> Ready Queue is full");
    }
}

//ready queue 에서 remove 하기 

processPointer remove_rQ(processPointer proc) {
    for (int i = 0; i < now_proc_num_rQ; i++) {
        if (readyQueue[i] == proc) {
            processPointer removed_proc = readyQueue[i]; //i 번째 지우고 싶다면 
            for (int j = i; j < now_proc_num_rQ - 1; j++) { 
                readyQueue[j] = readyQueue[j + 1]; //j 이후의 것들 한칸씩 당기기
            }
            readyQueue[--now_proc_num_rQ] = NULL;//마지막 배열 null로 
            return removed_proc;
        }
    } 
    return NULL;//queue empty
}

//waiting queue 초기화 
void init_wQ() {
    now_proc_num_wQ = 0;
    for (int i = 0; i < MAX; i++) {
        waitingQueue[i] = NULL;
    }
}

//waiting queue 메모리 회수 
void clear_wQ() {
    for (int i = 0; i < MAX; i++) {
        waitingQueue[i] = NULL;
    }
    now_proc_num_wQ = 0;
}

//waiting queue 에 insert
int insert_wQ(processPointer proc) {
    if (now_proc_num_wQ < MAX) {
        waitingQueue[now_proc_num_wQ++] = proc;
        return 1;
    }
    return 0;//full
}

//waiting queue remove
processPointer remove_wQ(processPointer proc) {
    for (int i = 0; i < now_proc_num_wQ; i++) {
        if (waitingQueue[i] == proc) { //지우고 싶은 부분
            processPointer removed_proc = waitingQueue[i];
            for (int j = i; j < now_proc_num_wQ - 1; j++) {
                waitingQueue[j] = waitingQueue[j + 1];
            }
            waitingQueue[--now_proc_num_wQ] = NULL;
            return removed_proc;
        }
    }
    return NULL;
}

//terminate 초기화
void init_term() {
    now_proc_num_ter = 0;
    for (int i = 0; i < MAX; i++) {
        termin[i] = NULL;
    }
}

//terminate 메모리 회수 
void clear_ter() {
    for (int i = 0; i < MAX; i++) {
        termin[i] = NULL;
    }
    now_proc_num_ter = 0;
}

//termin에 프로세스 넣기 
void insert_termin(processPointer proc) {
    if (now_proc_num_ter < MAX) {
        termin[now_proc_num_ter++] = proc; //terminate 에 새 proc 넣어주기 
    } else {
        puts("error no terminate\n");
        return;
    }
}


void print_queues() {
    puts("Job Queue:\n");
    for (int i = 0; i < now_proc_num_jQ; i++) {
        printf("[Process %d]: Arrival %d, CPU Burst %d\n", jobQueue[i]->PID, jobQueue[i]->arrival, jobQueue[i]->CPU_burst);
    }

    puts("Ready Queue:\n");
    for (int i = 0; i < now_proc_num_rQ; i++) {
        printf("[Process %d]: Arrival %d, CPU Burst %d\n", readyQueue[i]->PID, readyQueue[i]->arrival, readyQueue[i]->CPU_burst);
    }
}

processPointer FCFS() { //FCFS 알고리즘 
    processPointer earliestprocess = readyQueue[0]; // 먼저 도착한 process 찾기 , 먼저 job queue 에서 정렬=>ready queue 에 올라감
    if (earliestprocess != NULL) {
        if (runningproc != NULL) {//비선점이기 때문에 running 에 proc 이 있으면 대기 
            return runningproc;
        } else {//없으면 
            return remove_rQ(earliestprocess); //readyqueue 에서 running 할 proc 빼옴 
        }
    } else {
        return runningproc;// readyqueue 에 아무것도 없으면 그냥 running 상태로 돌아감
    }
}

//now_proc_num_jQ 가 종종 0이 되어 버려 따로 구하는 코드를 짜줌  
void update_job_queue_count() {
    now_proc_num_jQ = 0;
    for (int i = 0; i < MAX; i++) {
        if (jobQueue[i] != NULL) { //job queue 에 있으면 numm 증가 
            now_proc_num_jQ++;
        } else {
            puts("jobqueue empty");
        }
    }
}

//now_proc_num_rQ가 종종 0이 되어 버려 다시 count 해줌 
void update_ready_queue_count() {
    now_proc_num_rQ = 0;
    for (int i = 0; i < MAX; i++) {
        if (readyQueue[i] != NULL) {
            now_proc_num_rQ++;
        } else {
            puts("readyqueue empty");
        }
    }
}

//FCFS 가 끝나고 다시 sjf 릏 실행할때 job queue 에 아무것도 없어지는 일 발생(아마 초기화 이슈)
void reinsert_original_processes() {
    for (int i = 0; i < process_num; i++) {
        processPointer new_process = (processPointer)malloc(sizeof(process));
        if (new_process == NULL) {
            printf("Failed to allocate memory for process %d\n", orig[i].PID);
            exit(1);
        }
        *new_process = orig[i]; //orig 에 있는 초기에 create process 에서 생성한 data 옮겨주기 

        if (insert_jQ(new_process) == 0) { //이 data jo queue 에 insert
            free(new_process);
        }
    }
}

processPointer preem_SJF() { //선점 SJF 
    if (now_proc_num_rQ == 0) {
        return NULL;
    }

    processPointer shortestproc = readyQueue[0]; //ready queue 첫번째 부터 shortest 로 가정 
    if (shortestproc == NULL) {
        return NULL;
    }

    for (int i = 1; i < now_proc_num_rQ; i++) {
        if (readyQueue[i] != NULL) {
            if (readyQueue[i]->CPUremain_t < shortestproc->CPUremain_t) { //남아 있는 cpu bourst time 이 더 적다면 
                shortestproc = readyQueue[i];//shortest process 됨 
            } else if (readyQueue[i]->CPUremain_t == shortestproc->CPUremain_t) { //남아 있는 cpu_burst time 이 같다면 
                if (readyQueue[i]->arrival < shortestproc->arrival) { //arrival 로 정함 
                    shortestproc = readyQueue[i];//도착이 더 빠른 proc 이 shortest proc
                }
            }
        }
    }

    if (runningproc != NULL) { //running 에 proc 이 있으면 
        if (runningproc->CPUremain_t > shortestproc->CPUremain_t) { //rinning 에 있는 process 와 현재 shortest proc 중 누가 더 적게 남았는지 
            insert_rQ(runningproc); //shortest proc 이 더 적게 남았다면  running 도는 proc rQ에 넣어주고 
            return remove_rQ(shortestproc); //shortest는 rQ에서 빼줌 
        } else if (runningproc->CPUremain_t == shortestproc->CPUremain_t) {//같다면 
            if (runningproc->arrival > shortestproc->arrival) { //빨리 도착 선택 
                insert_rQ(runningproc);
                return remove_rQ(shortestproc);
            }
        }
        return runningproc;//runnning proc 으로 돌아가기 
    } else {
        return remove_rQ(shortestproc);//실행되는 proc 없으면 readyqueue 에서 가장 짧은 proc 선택 
    }
}


processPointer nonpreem_SJF() { //비선점 
    if (now_proc_num_rQ == 0) return NULL;
    if (runningproc != NULL) return runningproc; //running 잇으면 기다리고 
    processPointer shortestproc = readyQueue[0]; 
    for (int i = 1; i < now_proc_num_rQ; i++) {
        if (readyQueue[i]->CPUremain_t < shortestproc->CPUremain_t) {
            shortestproc = readyQueue[i];
        } else if (readyQueue[i]->CPUremain_t == shortestproc->CPUremain_t) {
            if (readyQueue[i]->arrival < shortestproc->arrival) {
                shortestproc = readyQueue[i];
            }
        }
    }
    return remove_rQ(shortestproc); //없으면 shortestproc runing 으로 
}


processPointer preem_Priority() { //우선순위 선점 
    if (now_proc_num_rQ == 0) return NULL;
    processPointer priorjob = readyQueue[0];//앞에 잇는 것 우선 작업이라 가정 
    for (int i = 1; i < now_proc_num_rQ; i++) {
        if (readyQueue[i]->priority < priorjob->priority) { //priority 가 작은게 우선 작업 
            priorjob = readyQueue[i];
        } else if (readyQueue[i]->priority == priorjob->priority) {//같다면 
            if (readyQueue[i]->arrival < priorjob->arrival) {//도착 빠른대로 
                priorjob = readyQueue[i];
            }
        }
    }
    if (runningproc != NULL) { //running 있다면 
        if (runningproc->priority > priorjob->priority) { //서로 우선순위 비교 
            puts("preemption in priority\n");
            insert_rQ(runningproc);//running 이 우선수위 낮다면 rQ로 넣고 
            return remove_rQ(priorjob); //prior 은 running 으로 
        } else if (runningproc->priority == priorjob->priority) { //같으면 
            if (runningproc->arrival > priorjob->arrival) {//도착 순서 
                puts("preemption in priority\n");
                insert_rQ(runningproc);//arrival 이 작은게 우선 =running proc rQ에 넣고 
                return remove_rQ(priorjob); //prior 은 running 으로 
            }
        }
        return runningproc;
    } else {
        return remove_rQ(priorjob);//running proc 없으면 바로 prior 실행
    }
}

processPointer nonpreem_Priority() { //비선점 
    if (now_proc_num_rQ == 0) return NULL;
    processPointer priorjob = readyQueue[0];
    for (int i = 1; i < now_proc_num_rQ; i++) {
        if (readyQueue[i]->priority < priorjob->priority) {
            priorjob = readyQueue[i];//prior 작은게 우선 작업 
        } else if (readyQueue[i]->priority == priorjob->priority) {//같다면 도착시간 
            if (readyQueue[i]->arrival < priorjob->arrival) {
                priorjob = readyQueue[i];
            }
        }
    }
    if (runningproc != NULL) {
        return runningproc;//비선점이어서 실행하고 잇는 proc 있으면 계속 실행
    } else {
        return remove_rQ(priorjob);//없으면 priorjob 
    }
}

processPointer RR() { //RR알고리즘 
    if (now_proc_num_rQ == 0) return NULL;
    processPointer earlyproc = readyQueue[0];
    if (earlyproc != NULL) {
        if (runningproc != NULL) {
            if (consumtime >= QUANTUM_TIME) { //소비시간이 3보다 크면 
                insert_rQ(runningproc); //rQ에 넣음
                return remove_rQ(earlyproc); //early proc 은 running 으로 
            } else {
                return runningproc;//running 있으면 그냥 running
            }
        } else {
            return remove_rQ(earlyproc);//없으면 그냥 earlyproc 실행
        }
    } else {
        return runningproc;
    }
}


//6가지의 알고리즘 중 select
processPointer pick_next(int alg) {
    processPointer pickproc = NULL;
    switch (alg) {
        case 1:
            pickproc = FCFS();
            break;
        case 2:
            pickproc = preem_SJF();
            break;
        case 3:
            pickproc = nonpreem_SJF();
            break;
        case 4:
            pickproc = preem_Priority();
            break;
        case 5:
            pickproc = nonpreem_Priority();
            break;
        case 6:
            pickproc = RR();
            break;
        default:
            puts("Invalid algorithm selected\n");
    }
    return pickproc;
}

//gantt chart 만들기 
void make_gantt_chart(int time, int pid) { //tome 과 pid 받아서 
    char s[10];
    //puts("<Gantt chart>\n");
    if (pid == -1) { //pid 없으면 idle 
        sprintf(s, "|idle");
    } 
    else {//있으면 running 
        sprintf(s, "|P%d", pid);
    }
    strcat(above_chart, s);//chart 와 time 나타내기 
    sprintf(s, "%4d", time);
    strcat(down_chart, s);
}

//running알고리즘 
void runningalg(int amount, int alg, int time_quantum) {
    processPointer nowProcess = NULL;
    static int reinserted = 0;
    if (alg != 1 && reinserted == 0) {
        reinsert_original_processes();//다시 job queue 에 origin data 넣음 
        remove_duplicate_pids_in_jQ();//중복 pid 제거 
        reinserted = 1;
    }

    sort_jQ();//job queue 정렬

    for (int i = 0; i < now_proc_num_jQ; i++) {//도착 =현재시간 
        if (jobQueue[i] != NULL && jobQueue[i]->arrival == amount) {// Arrival time equals current time => move from job queue to ready queue
            nowProcess = remove_jQ(jobQueue[i]);//job queue 에서 ready queue 로 
            if (nowProcess != NULL) {
                insert_rQ(nowProcess);
                i--;
            }
        }
    }

    processPointer pastproc = runningproc;
    runningproc = pick_next(alg);//알고리즘 셀렉 
    if (runningproc == NULL) {
        comput_idle++;//null 이면 idle 증가 
        make_gantt_chart(amount, -1); // Gantt 차트에 idle 시간 표시
        return;
    }

    if (pastproc != runningproc) {
        consumtime = 0;//proc 달라지면 consum 초기화 
    }

    for (int i = 0; i < now_proc_num_rQ; i++) {
        if (readyQueue[i]) { //ready queue 에 있으면 waiting time 과 turnaround 증가 
            readyQueue[i]->waiting_t++;
            readyQueue[i]->turnaround_t++;
        }
    }

    for (int i = 0; i < now_proc_num_wQ; i++) {
        if (waitingQueue[i]) {//waiting queue 에 있으면 
            waitingQueue[i]->turnaround_t++;//turn around 는 증가 
            waitingQueue[i]->IOremain_t--;//IO 가 실행중이므로 IO remain 은 감소 

            if (waitingQueue[i]->IOremain_t <= 0) {//remain ==0
                printf("time %d: (pid: %d) -> IO complete\n", amount, waitingQueue[i]->PID);
                insert_rQ(remove_wQ(waitingQueue[i--]));//다음 것 대기 큐에서 제거하고 준비 큐에 삽입
            }
        }
    }

    runningproc->CPUremain_t--; //running 중에는 cpu remain 감고 
    runningproc->turnaround_t++;//turnaround 증가 
    consumtime++;//RR

    make_gantt_chart(amount, runningproc->PID);//시간과 pid 로 간트 차트 

    printf("time %d: pid %d running\n", amount, runningproc->PID);
    if (runningproc->CPUremain_t <= 0) {//다 작업
        insert_termin(runningproc);//term =>끝냄
        printf("time %d: terminate process %d\n", amount, runningproc->PID);
        runningproc = NULL;
    } else {
        if (runningproc->IOremain_t > 0) { //IO remain 시간 이 남았다면 
            runningproc->IOremain_t -= 1;//1씩 감소 
            if (!insert_wQ(runningproc)) {
                printf("Failed to insert process %d into waiting queue\n", runningproc->PID);
                free(runningproc);
            } else {
                runningproc = NULL;//실행 중인 프로세스가 I/O 요청을 하면, 프로세스를 실행 상태에서 제거하고, I/O 요청 상태로 변경
                printf("time %d: -> IO request\n", amount);
            }
        }
    }
}


void analyize(int alg) {
    int wait_sum = 0;
    int turnaround_sum = 0;
    for (int i = 0; i < now_proc_num_ter; i++) {//ter 끝나지 않았다면 
        wait_sum += termin[i]->waiting_t;//끝난 wait 합 
        turnaround_sum += termin[i]->turnaround_t;//끝난 turnaround 합 
    }
    if (now_proc_num_ter != 0) {///끝난게 있으면 
        printf("Average waiting time: %d\n", wait_sum / now_proc_num_ter);
        printf("Average turnaround time: %d\n", turnaround_sum / now_proc_num_ter);
    }
    if (now_proc_num_ter != 0) {
        evaPointer newEval = (evaPointer)malloc(sizeof(struct evaluation));
        newEval->alg = alg;
        newEval->startTime = comput_start;
        newEval->endTime = comput_end;
        newEval->avg_turnaround_t = turnaround_sum / now_proc_num_ter;
        newEval->avg_waiting_t = wait_sum / now_proc_num_ter;
        evals[now_eval_num++] = newEval;
    }
    puts("------------------<Gantt chart>---------------------------------");
}

void startSimul(int alg, int time_quan, int count) {
    memset(above_chart, 0, sizeof(above_chart));
    memset(down_chart, 0, sizeof(down_chart));

    switch (alg) {
        case 1:
            puts("<FCFS Algorithm>");
            break;
        case 2:
            puts("<preem_SJF Algorithm>");
            break;
        case 3:
            puts("<nonpreem_SJF Algorithm>");
            break;
        case 4:
            puts("<preem_Priority Algorithm>");
            break;
        case 5:
            puts("<nonpreem_Priority Algorithm>");
            break;
        case 6:
            puts("<RR Algorithm>");
            break;
        default:
            return;
    }

    int initial_proc_num = process_num;

    for (int i = 0; i < count; i++) {
        runningalg(i, alg, 3);//count 만큼 돌기 (시간마다 )
        if (now_proc_num_ter == initial_proc_num) {//다 끝났으면 break
            break;
        }
    }
    comput_end = count - 1;
    analyize(alg);
    printf("%s\n%s\n", above_chart, down_chart);
    clear_jQ();
    clear_rQ();
    clear_ter();
    clear_wQ();
    free(runningproc);
    runningproc = NULL;
    consumtime = 0;
    comput_start = 0;
    comput_end = 0;
    comput_idle = 0;
}

void evaluate() {
    puts("\n<Evaluation>");
    for (int i = 0; i < now_eval_num; i++) {
        puts("======================================");
        switch (evals[i]->alg) {
            case 1:
                puts("<FCFS Algorithm>");
                break;
            case 2:
                puts("<preem_SJF Algorithm>");
                break;
            case 3:
                puts("<nonpreem_SJF Algorithm>");
                break;
            case 4:
                puts("<preem_Priority Algorithm>");
                break;
            case 5:
                puts("<nonpreem_Priority Algorithm>");
                break;
            case 6:
                puts("<RR Algorithm>");
                break;
            default:
                return;
        }
        puts("======================================");
        printf("Average waiting time: %d\n", evals[i]->avg_waiting_t);
        printf("Average turnaround time: %d\n", evals[i]->avg_turnaround_t);
    }
    puts("===============================================");
}

void Create_Process(void) {
    srand(time(NULL));
    process_num = (rand() % 5) + 4;
    printf("\nProcess number is %d\n", process_num);
    for (int i = 0; i < process_num; i++) {
        orig[i].arrival = (rand() % 10);
        orig[i].CPU_burst = (rand() % 10) + 1;
        orig[i].IO_burst = (rand() % 5) + 1;
        orig[i].priority = (rand() % process_num) + 1;
        int new_PID;
        do {
            new_PID = (rand() % 100) + 1;
        } while (is_PID_duplicate(new_PID, i));
        orig[i].PID = new_PID;
        orig[i].CPUremain_t = orig[i].CPU_burst;
        orig[i].IOremain_t = orig[i].IO_burst;
        orig[i].waiting_t = 0;
        orig[i].turnaround_t = 0;

        processPointer new_process = (processPointer)malloc(sizeof(process));
        if (new_process == NULL) {
            exit(1);
        }
        *new_process = orig[i];// orig 를 noew proc으로 

        if (insert_jQ(new_process) == 0) { //jQ에 옯기고 
            free(new_process);
        }
    }

    printf("\nEach process's() PID| arrival time | CPU burst time | IO burst time | Priority | CPU_remain_t )\n");
    for (int i = 0; i < process_num; i++) {
        puts("===============================================================================================");
        printf("     [process %d]:   |     %d       |       %d      |      %d       |     %d     |       %d     |\n", orig[i].PID, orig[i].arrival, orig[i].CPU_burst, orig[i].IO_burst, orig[i].priority, orig[i].CPUremain_t);
    }

    for (int i = 0; i < now_proc_num_jQ; i++) {
        processPointer newProcess = (processPointer)malloc(sizeof(struct process));
        newProcess->PID = jobQueue[i]->PID;
        newProcess->priority = jobQueue[i]->priority;
        newProcess->arrival = jobQueue[i]->arrival;
        newProcess->CPU_burst = jobQueue[i]->CPU_burst;
        newProcess->IO_burst = jobQueue[i]->IO_burst;
        newProcess->CPUremain_t = jobQueue[i]->CPU_burst;
        newProcess->IOremain_t = jobQueue[i]->IO_burst;

        newProcess->waiting_t = 0;
        newProcess->turnaround_t = 0;
        jobQueue[i] = newProcess;
    }

    sort_jQ();
}

int main() {
    init_jQ();
    init_rQ();
    init_wQ();
    init_evals();

    Create_Process();

    int amount = 100;
    startSimul(1, QUANTUM_TIME, amount);

    init_jQ();
    init_rQ();
    init_wQ();
    init_term();
    reinsert_original_processes();
    startSimul(2, QUANTUM_TIME, amount);

    init_jQ();
    init_rQ();
    init_wQ();
    init_term();
    reinsert_original_processes();
    startSimul(3, QUANTUM_TIME, amount);

    init_jQ();
    init_rQ();
    init_wQ();
    init_term();
    reinsert_original_processes();
    startSimul(4, QUANTUM_TIME, amount);

    init_jQ();
    init_rQ();
    init_wQ();
    init_term();
    reinsert_original_processes();
    startSimul(5, QUANTUM_TIME, amount);

    init_jQ();
    init_rQ();
    init_wQ();
    init_term();
    reinsert_original_processes();
    startSimul(6, QUANTUM_TIME, amount);

    evaluate();
    clear_rQ();
    clear_jQ();
    clear_ter();
    clear_wQ();
    clear_evals();

    return 0;
}


