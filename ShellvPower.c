/*Bibliotecas*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/*Cores*/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*Comandos*/
#define EXIT_CMD "exit"
#define CD_CMD "cd"
#define TILDE "~"

/*Tamanhos*/
#define TAM_CMD 4096
#define MAX_ARGS 4096
#define NAMES_SIZE 4096

/*Comparações*/
#define MISMATCH -1
#define TRUE 1
#define FALSE 0
#define MATCH 0
 
/*Variáveis Globais*/ 
  /*Vetor utilizado para armazenar os comandos simples*/
  char *comando_shell[MAX_ARGS];
  /*Usado como flag para sinalizar que o comando digitado possui flag*/
  int pipe_cmd = FALSE;
  /*K possui o total de comandos digitados quando o comando apresenta pipe */
  int k=0;
  /*Matriz utilizada para os comandos que possuem pipe*/
  char *commands[MAX_ARGS][MAX_ARGS];
  /*Sinaliza quando o usuário não digitou nada, apenas apertou ENTER*/
  int enter_pressionado = 0;

/*Flag para Ctrl-C e Ctrl Z*/
sig_atomic_t flag_notexecute = 0;


/*Declaração das funções*/
void imprimir_shell();
void ler_comando();
void executar(char **, char **);
void executar_cd();
void signal_init(struct sigaction);
void manipulador(int);


/*Função responsável por executar tanto os comandos com pipe e sem pipe*/
void executar(char **comando_shell,char **command){

    pid_t pd;
    pid_t pid;
    /*Checando a flag para não executar o comando*/
    if(flag_notexecute){
            fprintf(stdout,"\n");
            flag_notexecute = 0;
    }
    else if(enter_pressionado){
          enter_pressionado = 0;
    }
   /*Verificando se o comando digitado possui pipe*/
   else if(pipe_cmd == TRUE){
                    int p =0,j=0,status;
                    int fd[100][2];
                    // Visualizar o valor de K -> fprintf(stderr,"K :  %d\n", k);
                    /*Criando os pipes*/
                        for(p = 0;p < (k-1);p++){
                                pipe(fd[p]);
                            }
                    /*Para cada comando*/        
                       for(p = 0;p <= (k-1);p++){
                                pd = fork();
                                if(pd == 0){
                                   /*Se não for o primeiro comando*/
                                    if(p!=0){
                                        dup2(fd[p-1][0],0);
                                    }
                                    /*Se não for o último comando*/
                                    if(p!=(k-1)){
                                        dup2(fd[p][1],1);
                                    }
                                    /*Fecha todos os pipes*/
                                    for(j = 0;j < (k-1);j++){   
                                            close(fd[j][0]);
                                            close(fd[j][1]);
                                    }
                                    /*Executa o comando*/
                                    execvp(commands[p][0], commands[p]);
                                    fprintf(stdout, "Não foi possível executar o comando %s",commands[p][0]);
                                    fprintf(stdout, " : %s\n",strerror(errno));
                                    /*Se der erro, finaliza o programa*/
                                    exit(0);
                                }
                            }
                            /*Fecha todos os pipes*/
                            for(p = 0;p < (k-1);p++){
                                close(fd[p][0]);
                                close(fd[p][1]);
                            }
                            /*Espera os processos finalizarem*/
                            for(p = 0; p < (k);p++){
                                wait(&status);
                                waitpid(pd,&status,0);
                            }            
                         
         }
         /*Se o comando não possuir pipe*/   
         else{
               if (strcmp(comando_shell[0],EXIT_CMD)==MATCH){
                    exit(0);
                }else if(strcmp(comando_shell[0],CD_CMD)==MATCH){
                    executar_cd(comando_shell);
                }else{               
                     pid = fork();
                     /* Visualizar o comando digitado */ //fprintf(stdout,"Comando : %s\n", comando[0]);
                     if (pid < 0){
                        // Não foi possível realizar o fork
                        printf("Error");
                     }else if (pid > 0){
                        // Pai 
                        int status;
                        // Espera pelo filho
                        wait(&status);
                        waitpid(pid,&status,0); 
                      }
                        else{ 
                             // Filho
                             // Executa o comando
                            execvp(comando_shell[0],comando_shell);
                            fprintf(stdout, "Não foi possível executar o comando %s",comando_shell[0]);
                            fprintf(stdout, " : %s\n",strerror(errno));
                            exit(0);
                    }
                  }
            }
}



void executar_cd(){

    // Caso o comando cd vier sem pasta
    if (comando_shell[1]==NULL || strcmp(comando_shell[1],TILDE) == MATCH){
       /*Acessa o diretório home*/
        chdir(getenv("HOME"));
    }// Caso o usuário digitar um diretório inválido
    else if(chdir(comando_shell[1])== MISMATCH){
        fprintf (stderr,"Erro ao executar o comando %s",comando_shell[0]);
        fprintf(stderr," : %s\n",strerror(errno));
    }      
}

 
//void lercomando(char *comando_shell[])
void ler_comando(){

    //char *command;
    char command[TAM_CMD];
    char *ptr_pipe;
    size_t size;

 
    fgets(command,TAM_CMD,stdin);
    ptr_pipe = strstr(command,"|");
    /*Caso o ponteiro for nulo, o usuário não digitou um comando com pipe*/
     if(ptr_pipe == NULL){ 
      /*Detectando o Ctrl-D*/
      /*Ctrl-D envia um "sinal" de EOF*/
      /*Portanto, verifica-se se o resultado do fgets é EOF*/  
        if (feof(stdin)){
            /*Imprimir nova linha por questão de padronização*/printf("\n");
            /* Sair do programa*/exit(0);
         }
        /*Caso o usuário digitou algo */
          if(strlen(command) > 1 ){
             
             /*Sinalizando que o comando não possui pipe*/
             pipe_cmd = FALSE;
             command[strlen(command)-1] = '\0';
             //Obtendo todos os argumentos
             int i= 0;

             /*Removendo espaços iniciais*/
             while(isspace(command[i])){i++;}
             memmove(command,command+i,strlen(command));

             /*É necessário zerar a variável de controle para o laço abaixo*/
             i=0;

             /*Obtendo o nome do programa e seus argumentos*/
             char *token = strtok(command, " ");
             while(token != NULL){
                comando_shell[i++] = token;
                token = strtok(NULL, " ");
             }
             /*Finalizando a string*/
             comando_shell[i] = '\0';  
            }
            else{
              /*Sinaliza ao laço do executar que não é necessário imprimir nova linha*/
              enter_pressionado = 1;
              /*A string comando recebe nenhum comando digitado*/
              command[0] = '\0';
            }
        }
        /*Caso o ponteiro for diferente de nulo, o usuário digitou um comando com pipe*/
        else{
            /*Sinaliza para o laço do main que a próxima instrução possui pipe*/  
            pipe_cmd = TRUE;
            
            command[strlen(command)-1] = '\0';
            char *cmd = strtok(command,"|");
  
            /*Variáveis de controle*/
            int i=0;
            int j =0;
            k=0;
            int NUM_COMANDOS =0;
      
            /*Separando os comandos pelo pipe*/
            commands[i++][k] = cmd;
            cmd = strtok(NULL,"|");

            while(cmd != NULL){        
               while(cmd != NULL){
                 /*Necessário incrementar o ponteiro pois está apontando para o KEYSPACE*/
                 /*Ex : ls -l | wc -c*/
                 /*A nossa sintaxe utilizada implica o pipe separado por espaços*/
                 commands[i++][k] = ++cmd; 
                 cmd = strtok(NULL,"|");
              }
              cmd = strtok(NULL,"|"); 
           }
            /*"Finalizando" o ponteiro de char*/
            commands[i][k] = NULL;

            /*Imprimir os comandos (Visualizar se a recuperação foi feita com sucesso)*/
            //fprintf(stderr,"Comando1:%s\nComando2:%s\n",commands[0][0],commands[1][0]);
        
            /*NUM_COMANDOS recebe o total de comandos digitados Ex : ((ls -l / | sort | grep a )) -> NUM_COMANDOS = 3*/
            NUM_COMANDOS = i - 1;

            /*Zerando as variáveis de controle da matriz*/
            i=0;
            k=0;

            /*Recuperando os argumentos de cada comando*/
            char *token;
            while(k<=NUM_COMANDOS){
                token = strtok(commands[k][0]," ");
                while(token !=NULL){
                  commands[k][i++] = token;
                  token = strtok(NULL," ");
                }
                commands[k][i] = NULL;
                k++;
                i=0; 
            }

            //fprintf(stderr,"Argumento 1 : %s\nArgumento 2 : %s\n",commands[0][1],commands[1][1]);          

          }
}

/*Função responsável por imprimir a Shell nos padrões requisitados*/
/*Foi adicionado cor ao fprintf, por motivos estéticos somente*/
void imprimir_shell(){

    char *dir_home = getenv("HOME");
    char dir_atual[NAMES_SIZE];   
    getcwd(dir_atual, NAMES_SIZE);
    char host[NAMES_SIZE];
    int lenhome = strlen(dir_home);

    /*Compara se o diretório atual faz parte do diretório home*/
    if (strncmp(dir_home, dir_atual, strlen(dir_home)) == MATCH){
        /*Função para realizar o replace de home no current_dir*/       
        memmove(dir_atual, dir_atual+lenhome-1, strlen(dir_atual));  
        /*O último do caractere de dir_home recebe ~*/
        dir_atual[0]='~';
 }
    //Obtendo o nome do usário
    gethostname(host, NAMES_SIZE);
    //Imprimir a shell
    fprintf(stdout,"%s[MySh] %s@%s%s:%s%s%s$ ",/* Obtem o ambiente*/ ANSI_COLOR_GREEN ,getenv("LOGNAME") ,host,ANSI_COLOR_RESET ,
    ANSI_COLOR_BLUE,dir_atual,ANSI_COLOR_RESET);

}

/*Função que está configurando o Ctrl-C e o Ctrl-Z*/
void signal_init(struct sigaction sinal){

    //Aloca memória
    memset (&sinal, 0, sizeof (sinal));
    
    //Recebe handler
    sinal.sa_handler = &manipulador;
    
    // Ctrl+Z faz nada
    sigaction (SIGTSTP, &sinal, NULL); 
    // Ctrl+C faz nada
    sigaction (SIGINT, &sinal, NULL); 
 
}
 
void manipulador (int signal_number){

  // Levanta a flag para sinalizar que não é necessário realizar nenhum comando 
    flag_notexecute = 1;
}

int main(int argc, char const *argv[]){

    printf("%sMyShell -- 2019\n%sMarcos P.Camargo -- Vinicius Dornelles\n", ANSI_COLOR_CYAN,ANSI_COLOR_BLUE);
    /*Estrutura sigaction*/
    struct sigaction sinal;
    //Chama função para inicializar os sinais
    signal_init(sinal);
    // Loop infinito até o comando "exit" ser executado pela função executar(comando_shell,commandos[0])
    // ou até o Ctrl-D ser capturado por ler_comando()
    while(TRUE){
        //Imprime o prompt
        imprimir_shell();
        //Ler o comando
        ler_comando();
        //Executar
        executar(comando_shell,commands[0]);
    }
}
