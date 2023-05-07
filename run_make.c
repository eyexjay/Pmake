#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "pmake.h"

static int evaluate(Rule *rule, Rule *target_rule, int p);
static int compare_time(Rule *dep_rule, Rule *target_rule);
static void execute(Action *action);
static void execute_actions(Rule *target_rule);
static void evaluate_first(Rule *rule, int p);
static char * filename (char * name);

/* Evaluate the rule in rules corresponding to the given target.
   If target is NULL, evaluates the first rule instead.
   If pflag is 0, evaluates each dependency in sequence.
   If pflag is 1, then evaluates each dependency in parallel (by creating one 
   new process per dependency). 
 */
void run_make(char *target, Rule *rules, int pflag) {
  if (target == NULL){ 
    evaluate_first(rules, pflag); 
  }
  else{
    Rule *curr = rules;
    while (curr != NULL){
      if(strcmp(curr -> target, target) == 0){
        break;
      }
      curr = curr ->next_rule;
    }

    if (curr != NULL){ evaluate_first(curr, pflag); }
    else{
      printf("Target %s not found\n", target); 
    }
    
  }

}

void execute_actions(Rule *target_rule){
  Action *curr_act = target_rule -> actions;
  while(curr_act != NULL){
    execute(curr_act);
      curr_act = curr_act ->next_act;
    }
}

void execute(Action *action){
  char ** arg = action->args;    
  pid_t pid = fork();
  if (pid < 0){
    perror("fork"); 
    exit(1); 
  }
  else if (pid == 0){ //child
    for (int i = 0; arg[i] != NULL; i++){
      printf("%s ", arg[i]);
    }
    printf("\n");
    execvp(arg[0], arg);
    perror("exec");
    exit(1);
  }
  else{ //parent
    int status;
    if(wait(&status) == -1){
      perror("wait");
      exit(1);
    }
    else if (WIFSIGNALED(status)){ 
      exit(1);
    }
  }
}

void evaluate_first(Rule *rule, int p){
  int ex_flag = 0;
  Dependency* dep = rule->dependencies;
  if (dep == NULL) { if (rule->actions != NULL) {ex_flag = 1; } }
  else{
    while (dep != NULL){ 
      ex_flag = ex_flag+evaluate(dep->rule, rule, p);
      dep = dep->next_dep;
    }
  }
  if (ex_flag > 0) { execute_actions(rule);}
}

int evaluate(Rule *rule, Rule *target_rule, int p){
  Dependency *cur_dep = rule -> dependencies;
  Rule *cur_rule = rule ;

  pid_t pid = -1;
  while(cur_dep != NULL){ //iterating and evaluating each dependency
    if (p) {
      pid = fork();
      if (pid ==0) { evaluate_first(cur_rule, p); }
      else if (pid < 0) { perror("fork"); exit(1); }
      else {
        int status;
        if(wait(&status) == -1){
          perror("wait");
          exit(1);
        }
        else if (WIFSIGNALED(status)){exit(1); } 
      }
    }
    else {evaluate_first(cur_rule, p); }
    cur_rule = cur_dep -> rule;
    cur_dep = cur_dep -> next_dep;        
  } 
  if(compare_time(rule, target_rule)){return 1;}
  return 0;
}

char *filename(char *name){ 
  char *relative_path = "./";
  int len = strlen(name) + 3;
  char *fname = malloc(sizeof(char) * len);
  if (fname == NULL){ // error handeling  
        perror("malloc");
        exit(1);   
  } 
  strcpy(fname, "\0");
  strcat(fname, relative_path);
  strcat(fname, name);
  return fname;
}

int compare_time(Rule *dep_rule, Rule *target_rule){
  struct stat dep_stat;
  struct stat target_stat;

  char *dep_filename = filename(dep_rule -> target);
  char *target_filename = filename(target_rule -> target);

  //if doesnt exist dependency exists
  if(stat(dep_filename, &dep_stat) == -1){
      free(dep_filename);
      free(target_filename);
      return 1;
  }

  if(stat(target_filename, &target_stat) == -1){
    free(dep_filename);
    free(target_filename);
    return 1;
        
  }

  if ((dep_stat.st_mtim.tv_sec) > (target_stat.st_mtim.tv_sec)){
    free(dep_filename);
    free(target_filename);
     return 1;
  }
  else if ((dep_stat.st_mtim.tv_sec) == (target_stat.st_mtim.tv_sec)){
    if ((dep_stat.st_mtim.tv_nsec) > (target_stat.st_mtim.tv_nsec)){
      free(dep_filename);
      free(target_filename);
      return 1; 
    }
    else {
      free(dep_filename);
      free(target_filename);
      return 0; 
    }
  }
  else{
    free(dep_filename);
    free(target_filename);
    return 0;
  }
}
  