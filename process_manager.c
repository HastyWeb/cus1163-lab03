#include "process_manager.h"

/*
 * Function 1: Basic Producer-Consumer Demo
 * Creates one producer child (sends 1,2,3,4,5) and one consumer child (adds them up)
 */
int run_basic_demo(void) {
    int pipe_fd[2];
    pid_t producer_pid, consumer_pid;
    
    printf("\nParent process (PID: %d) creating children...\n", getpid());
    
    // Create a pipe for communication
    // Use pipe(pipe_fd), check return value
    if (pipe(pipe_fd) == -1) {
         perror("pipe");
         return -1;
    }


    //Fork the producer process
    //HINT: producer_pid = fork();  
    producer_pid = fork();
    if(producer_pid == 0) {
      //Child must close pipe_fd[0] (read end)
      close(pipe_fd[0]);
      //Child calls: producer_process(pipe_fd[1], 1);
      //start with number 1
      producer_process(pipe_fd[1], 1);
    } else if (producer_pid > 0) {
      //parent prints: "Created producer child (PID: %d)"
      printf("Created producer child (PID: %d)\n", producer_pid);
    } else {
      //fork failure handling
      perror("fork");
      close(pipe_fd[0]);
      close(pipe_fd[1]);
      return -1;
    }


    //Fork the consumer process
    //HINT: consumer_pid = fork();
    consumer_pid = fork();
    if(consumer_pid == 0) {
      //Child must close pipe_fd[1] (write end)
      close(pipe_fd[1]);
      //Child calls: consumer_process(pipe_fd[0], 0);  
      //Pair ID 0 for basic demo
      consumer_process(pipe_fd[0], 0);
    } else if (consumer_pid > 0) {
      //Parent prints: "Created consumer child (PID: %d)"
      printf("Created consumer child (PID: %d)\n", consumer_pid);
    } else {
      //fork failure handling
      perror("fork");
      close(pipe_fd[0]);
      close(pipe_fd[1]);
      return -1;
    }



    //Parent cleanup - close pipe ends and wait for children
    //HINT: close(pipe_fd[0]); close(pipe_fd[1]);
    //store each termination status seperately
    int producer_status;
    int consumer_status;
    
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    
    //Use waitpid() twice to wait for both specific children
    
    //waiting for producer
    //add error handling
    if(waitpid(producer_pid, &producer_status, 0) == -1) {
      perror("waitpid");
      return -1;
    }
    
    //USE TERNARY OPERATOR FOR WIFEXITED (?)
    printf("Producer child (PID: %d) exited with status %d\n", producer_pid, WIFEXITED(producer_status) ? WEXITSTATUS(producer_status): 1);
    
    //waiting for consumer
    //add error handling
    if(waitpid(consumer_pid, &consumer_status, 0) == -1) {
      perror("waitpid");
      return -1;
    }
    
    //USE WIFEXITED again (ternary ? operator)
    printf("Consumer child (PID: %d) exited with status %d\n", consumer_pid, WIFEXITED(consumer_status) ? WEXITSTATUS(consumer_status) : 1);

    return 0;
}

/*
 * Function 2: Multiple Producer-Consumer Pairs
 * Creates multiple pairs: pair 1 uses numbers 1-5, pair 2 uses 6-10, etc.
 */
int run_multiple_pairs(int num_pairs) {
    pid_t pids[10]; // Store all child PIDs
    int pid_count = 0;
    //add status int (used later)
    int statuses[10];

    printf("\nParent creating %d producer-consumer pairs...\n", num_pairs);

    //Create multiple producer-consumer pairs
    //HINT: Use a for loop from i=0 to i<num_pairs
    for(int i = 0; i < num_pairs; i++) {
      //re-used each time
      int pipe_fd[2];
      pid_t producer_pid, consumer_pid;
      int producer_status;
      int consumer_status;
      //Print "=== Pair %d ===" for each pair
      printf("\n=== Pair %d ===\n", i + 1);
      
      //Create a new pipe
      if(pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
      }
      //Fork producer: calls producer_process(write_fd, i*5 + 1)
      //So pair 1 starts with 1, pair 2 starts with 6, pair 3 starts with 11
      producer_pid = fork();
      if(producer_pid == 0) {
        close(pipe_fd[0]);
        producer_process(pipe_fd[1], i * 5 +1);
      } else if (producer_pid > 0) {
          //Store both PIDs in pids array, increment pid_count
          pids[pid_count++] = producer_pid;
      } else {
        perror("fork");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
      }
      
      //Fork consumer: calls consumer_process(read_fd, i+1)
      consumer_pid = fork();
      if(consumer_pid == 0) {
        close(pipe_fd[1]);
        consumer_process(pipe_fd[0], i + 1);
      } else if(consumer_pid > 0) {
          //Store both PIDs in pids array, increment pid_count
          pids[pid_count++] = consumer_pid;
      } else {
          perror("fork");
          close(pipe_fd[0]);
          close(pipe_fd[1]);
          return -1;
      }
      
      //parent closes both pipe ends
      close(pipe_fd[0]);
      close(pipe_fd[1]);
      
      //wait for producer then consumer to follow sample output
      if(waitpid(producer_pid, &producer_status, 0) == -1) {
        perror("waitpid");
        return -1;
      }
      if(waitpid(consumer_pid, &consumer_status, 0) == -1) {
        perror("waitpid");
        return -1;
      }
      
      //store exit statues to print later
      //producer PID
      statuses[pid_count - 2] = producer_status;
      //consumer PID
      statuses[pid_count - 1] = consumer_status;
    }
    
    
    //wait for all children
    //hint use for loop
    //print exit status for each child
    printf("\nAll pairs completed successfully!\n");
    for(int i = 0; i < pid_count; i++) {
      //use WEXITSTATUS to get exit code
      printf("Child (PID: %d) exited with status %d\n", pids[i], WEXITSTATUS(statuses[i]));
    }
    return 0;
}

/*
 * Producer Process - Sends 5 sequential numbers starting from start_num
 */
void producer_process(int write_fd, int start_num) {
    printf("Producer (PID: %d) starting...\n", getpid());
    
    // Send 5 numbers: start_num, start_num+1, start_num+2, start_num+3, start_num+4
    for (int i = 0; i < NUM_VALUES; i++) {
        int number = start_num + i;
        
        if (write(write_fd, &number, sizeof(number)) != sizeof(number)) {
            perror("write");
            exit(1);
        }
        
        printf("Producer: Sent number %d\n", number);
        usleep(100000); // Small delay to see output clearly
    }
    
    printf("Producer: Finished sending %d numbers\n", NUM_VALUES);
    close(write_fd);
    exit(0);
}

/*
 * Consumer Process - Receives numbers and calculates sum
 */
void consumer_process(int read_fd, int pair_id) {
    int number;
    int count = 0;
    int sum = 0;
    
    printf("Consumer (PID: %d) starting...\n", getpid());
    
    // Read numbers until pipe is closed
    while (read(read_fd, &number, sizeof(number)) > 0) {
        count++;
        sum += number;
        printf("Consumer: Received %d, running sum: %d\n", number, sum);
    }
    
    printf("Consumer: Final sum: %d\n", sum);
    close(read_fd);
    exit(0);
}
