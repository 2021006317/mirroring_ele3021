// def queue and linkedList

typedef struct Process{
    int pid;
    int lt; // local tick
    int pr; // priority
    int ql; // queue level
    Node* next;
}Node;

typedef struct queue{
    Node* front;
    Node* rear;
    int count;
    int level;
    int tq; // time quantum
}q;

void init_queue(q* queue){ // init queue
    q que = *queue;
    que.front = 0;
    que.rear = 0;
    que.count = 0;
}

int isEmpty(q* queue){ // check if queue is empty
    q que = *queue;
    if (que.count==0) return 1;
    else return 0;
}

void push(q* queue, Node* newRear){ // 큐 맨 뒤에 삽입
    q que = *queue;  
    Node* oldRear = que.rear;
    Node rear = *oldRear;

    oldRear->next = newRear;
    que.rear = newRear;
    que.count++;
}

int pop(q* queue){ // 맨 앞 요소 뱉으면서 삭제
    q que = *queue;  
    if (isEmpty(queue) == 0) {
        Node* oldFront = que.front;
        Node oldFrontNode = * oldFront;
        Node* newFront = oldFront->next;
        que.front = newFront;
        que.count--;
        return oldFront;
    }
    else{
        //!error 아님?
    }
}

int find_end(q* queue){ // return queue rear address
    q que = *queue;   
    Node* rear_addr = que.rear;
    // Node rear = *rear_addr;
    // int element[] = {rear.pid, rear.lt, rear.pr, rear.ql};
    return rear_addr;
}

int find_front(q* queue){ // return queue front address
    q que = *queue;   
    Node* front_addr = que.front;
    // Node front = *front_addr;
    // int element[] = {front.pid, front.lt, front.pr, front.ql};
    return front_addr;
}

void
setTq(q* queue, int num)
{
    q que = *queue;
    que.tq = 2 * que.level + 4;
    printf('queue level is %d and time quantum is %d', que.level, que.tq);
}

void
setPriority(int pid, int priority)
{
    if (priority <0 || priority >3) return;
    else{
        proc.pid = pid
        proc.pr = priority
    }
}

void
scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for(;;){
        sti();

        acquire(&ptable.lock);
        for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
            if(p->state != RUNNABLE){
                continue;
            }

        }
    }
}