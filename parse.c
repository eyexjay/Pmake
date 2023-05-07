#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
  
#include "pmake.h"  
  
static int is_a_target(char *line);  
static Rule *existing_rule(char *target, Rule *rule);  
static Dependency *add_dependencies(char *line);  
static char **create_args(char *line);  
static Action *last_action(Rule *rule);  
static Rule *create_rule(char * name);  

char * separator = " \r\n\t"; 
  
/* Read from the open file fp, and create the linked data structure 
   that represents the Makefile contained in the file. 
*/  
Rule *parse_file(FILE *fp) {  
      
    Rule *rule = NULL;  
    Rule *head = NULL;  
    Rule *prev = NULL;  
  
    char line[MAXLINE];
  
    int is_target = 0;  
  
    while(fgets(line, MAXLINE, fp) != NULL){   
        is_target = is_a_target(line);   
  
        if (is_target){    
            char *token = strtok(line, separator);   
              
            rule = existing_rule(token, head);  
            if (rule == NULL){   
                rule = create_rule(token);  
            }   
            rule->dependencies = add_dependencies(token);  
  
  
            if (head == NULL){   
                head = rule;  
                prev = rule;  
            }  
            else{  
                prev->next_rule = rule;  
                prev = rule;  
            }  
        }   
        else if ((is_comment_or_empty(line)== 0) && (line[0] == '\t')){  
  
            Action *act_tail = last_action(rule);  
            if (act_tail == NULL){  
                rule ->actions = malloc(sizeof(Action));  
                rule -> actions -> args = create_args(line);  
                rule -> actions -> next_act = NULL;  
            }  
            else{  
                act_tail ->next_act = malloc(sizeof(Action));  
                act_tail -> next_act-> args = create_args(line);  
                act_tail -> next_act -> next_act = NULL;  
  
            }  
        }  
  
    }   
  
    return head;  
}  
  
  
/********* Helper Functions *********/
/* Return 1 if the line is a target line. Return 0 otherwise.
*/
int is_a_target(char *line){  
    if (is_comment_or_empty(line) == 1){
        return 0;  
    }    
    if (line[0] == '\t'){  
        return 0;  
    }  
  
    if (line[0] == '\r'){
        return 0;  
    }  
    return 1;   
}  

/* Checks if the target is an existing rule in the rules list.
   If it exists, return the rule. Return Null otherwise. 
*/  
int check_rule(char *target, Rule *curr)  
    {  
        if (strcmp(curr->target, target)==0) {return 1;}  
        else {return 0;}  
    }  
  
/* Creates and returns dependeincies list. 
*/
Rule *existing_rule(char *target, Rule *rule)  
{  
    Rule *curr = rule;  
    Dependency *dep = NULL;  
    while(curr != NULL){  
        if (check_rule(target, curr)){ return curr;}  
        else {  
            dep = curr->dependencies;  
            while (dep !=NULL)  
            {  
                if (check_rule(target, dep->rule)) {return dep->rule; }  
                else {dep = dep->next_dep;}  
            }  
        }  
        curr = curr->next_rule;  
    }  
    return NULL;  
  
}  
  
/* Return pointer to last rule. 
*/  
Rule *end_of(Rule *head){  
    Rule *curr_rule = head;  
    if (curr_rule != NULL){  
        while (curr_rule -> next_rule != NULL){  
            curr_rule = curr_rule -> next_rule;  
        }  
    }  
    return curr_rule;  
}  
  
/* Creates and returns dependencies list. 
*/  
Dependency *add_dependencies(char *token){  
    // create dependencies  
    Dependency *prev = NULL;   
    Dependency *dep = NULL;  
    Dependency *dep_head = NULL;  
    int found_colon = 0;  
  
    while (token != NULL){   
        if(strcmp(token, ":") == 0) {  
            found_colon = 1;                  
        }   
        else if (found_colon){ // dependencies   
            dep = malloc(sizeof(Dependency));      
            dep-> rule = create_rule(token);  
            dep-> next_dep = NULL;  
  
            if (dep_head == NULL){  
                 dep_head = dep;  
                 prev = dep;  
             }  
             else{  
                prev -> next_dep = dep;  
             }  
  
        }  
        token = strtok(NULL, separator);  
    }  
    return dep_head;  
}  
  
/* Creates and returns dependeincies list. 
*/  
Action *last_action(Rule *rule){  
    Action *curr_act = rule ->actions;  
    if (curr_act != NULL){  
        while (curr_act -> next_act != NULL){  
            curr_act = curr_act -> next_act;  
        }  
    }  
    return curr_act;  
}  
  
/* Creates and returns a rule with name target element.
   All other elements are initialized to Null. 
*/
Rule *create_rule(char * name)  
{  
    Rule *rule = NULL;  
    rule = malloc(sizeof(Rule));  
    rule->target = malloc(sizeof(char*));  
    strcpy(rule->target, name);  
    rule-> dependencies = NULL;  
    rule-> actions = NULL;  
    rule-> next_rule = NULL;  
    return rule;  
}  
  
/* Returns the size required to create the args element
   for a given action line.
*/
int arg_length(char *line){  
    int count = 0;  
    for (int i = 0; i < strlen(line); i++){  
        if (line[i] == ' ') {  
            count +=1;  
        }  
    }  
    return count + 2;  
}  
  
/* Creates and returns args element for an action line.
*/  
char **create_args(char *line){  
    int len = arg_length(line);  
    char *token = strtok(line, " ");             
  
    char **evp_args = malloc(sizeof(char *) * len);  
  
    int i = 0;  
    token += 1; 
    while (token !=NULL){  
        evp_args[i] = malloc(sizeof(char) * (strlen(token)+ 1)); // + 1 to include null term  
        strncpy(evp_args[i], token, strlen(token) + 1);  
        i +=1;   
        token = strtok(NULL, separator);  
    }  
    evp_args[i] = '\0';   
    return evp_args;  
  
}  
  
/* Print the list of actions */  
void print_actions(Action *act) {  
    while(act != NULL) {  
        if(act->args == NULL) {  
            fprintf(stderr, "ERROR: action with NULL args\n");  
            act = act->next_act;  
            continue;  
        }  
        printf("\t");  
  
        int i = 0;  
        while(act->args[i] != NULL) {  
            printf("%s ", act->args[i]) ;  
            i++;  
        }  
        printf("\n");  
        act = act->next_act;  
    }  
}  
  
  
/* Print the list of rules to stdout in makefile format. If the output 
   of print_rules is saved to a file, it should be possible to use it to 
   run make correctly. 
 */  
void print_rules(Rule *rules){  
    Rule *cur = rules;  
  
    while (cur != NULL) {  
        if (cur->dependencies || cur->actions) {  
            // Print target  
            printf("%s : ", cur->target);  
  
            // Print dependencies  
            Dependency *dep = cur->dependencies;  
            while (dep != NULL){  
                if(dep->rule->target == NULL) {  
                    fprintf(stderr, "ERROR: dependency with NULL rule\n");  
                }  
                printf("%s ", dep->rule->target);  
                dep = dep->next_dep;  
            }  
            printf("\n");  
  
            // Print actions  
            print_actions(cur->actions);  
        }  
        cur = cur->next_rule;  
    }  
}  
  
  
/* Return 1 if the line is a comment line. 
   Return 0 otherwise. 
 */  
int is_comment_or_empty(const char *line) {  
    for (int i = 0; i < strlen(line); i++){  
        if (line[i] == '#') {  
            return 1;  
        }  
        if (line[i] != '\t' && line[i] != ' ') {  
            return 0;  
        }  
    }  
    return 1;  
}  
  
/* Convert an array of args to a single space-separated string in buffer. 
   Returns buffer.  Note that memory for args and buffer should be allocted 
   by the caller. 
 */  
char *args_to_string(char **args, char *buffer, int size) {  
    buffer[0] = '\0';  
    int i = 0;  
    while (args[i] != NULL) {  
        strncat(buffer, args[i], size - strlen(buffer));  
        strncat(buffer, " ", size - strlen(buffer));  
        i++;  
    }  
    return buffer;  
}  