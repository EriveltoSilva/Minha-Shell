/*----------------------------------------------------------------------------------------------------------------------
*Objectivo: Fazer um mini interpretador de comandos Linux em C.

*Feito por:
*** Cesar Mateque Aurelio, 1000026516.
*** Erivelto Clenio da Costa e Silva, 1000026877.
*** Teresa Filipe Augusto, 1000027093.
*Para: Universidade Cat�lica de Angola
*Ano: 2�, primeiro semestre
*Disciplina: Sistemas Operativos 2
*Professora: L�gia Chiuale-----------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>	//Funções de ENtrada e Saída Padrão
#include <errno.h>	//Exibição de erros
#include <fcntl.h>	//Funções e constantes para utilização de arquivos
#include <stdlib.h>	//Funções Gerais
#include <string.h>	//Utilização de funções para strings
#include <unistd.h> 	//Criação de processos
#include <sys/wait.h> 	//Utilização de wait
#include <signal.h>	//Funções para uso de sinais 

#define TAMANHO_LINHA 100
#define TAMANHO_ARGUMENTOS 15
#define TRUE 1
#define FALSE 0
#define REDIRECIONAMENTO_ENTRADA 0

typedef enum {FOREGROUND=0, BACKGROUND=1}TPlano; 
typedef enum {VAZIO=0, NAO_VAZIO=1} TAbertura; 
typedef enum{REDIRECIONAMENTO_SAIDA=1,NAO_TEM_REDIRECIONAMENTO=2}TRedirecionamento;

typedef struct 
{
	int processosPausados[10];
	int nElem;
}TProcesso;

TProcesso ficha;

/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Limpar a tela do prompt
Recebe: Nada
Retorna:Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void limparTela()
{
	int statusEspera;
	int processo = fork();
	
	if(processo< 0)	
		printf("ERRO AO CRIAR O PROCESSO\n");
	else if(processo>0)
		wait(&statusEspera);
	else
		execl("/bin/clear", "clear",NULL);	
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Ler um comando digitado pelo usuário
Recebe: um vector de char
Retorna:Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void lerComando(char comando[])
{
	printf("\nmsh->");
	fgets(comando, TAMANHO_LINHA, stdin);
	comando[strlen(comando) - 1] = '\0';
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Compara dois comandos(strings)
Recebe: dois vectores char
Retorna: um valor interiro, 0 - se forem diferentes, 1 - se forem iguais 
--------------------------------------------------------------------------------------------------------------------------------*/
int isComando(char *comando, char *comandoRecebido)
{
	return (strcmp(comando, comandoRecebido)==0);
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Verirfica se o comando está fazio
Recebe: um vector char
Retorna: Um valor inteiro, 1 - caso comando esteja vazio , 0 caso contrario
--------------------------------------------------------------------------------------------------------------------------------*/
int isComandoVazio(char *comando)
{
	return(strcmp(comando, "")==0);
}
/*-------------------------------------------------------------------------------------------------------------------------------
Objectivo: guardar o ultimo comando digitado
Recebe: dois vectores de char(strings)
Retorna:Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void guardarUltimoComando(char *comandoAnterior, char *comando)
{
	strcpy(comandoAnterior, comando);
}
/*-------------------------------------------------------------------------------------------------------------------------------
Objectivo: apaga o último comando digitado
Recebe: um vertor de char(string)
Retorna:Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void apagarComando(char *comando)
{
	strcpy(comando, "");
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: verificar se um comando deve ser executado em foreground ou background
Recebe: um vertor de char(string)
Retorna: um valor inteiro. 1 - caso tenha &(background), 0 - caso não tenha (foreground)
--------------------------------------------------------------------------------------------------------------------------------*/
int isBackground(char *comandoLido)
{
	return (strchr(comandoLido, '&')!=NULL);
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Verificar se o camando tem redirecionamento e tipo caso tenha
Recebe: um vector de char(string)
Retorna: um valor inteiro. 0 - redirecionamento de entrada , 1 - redirecionamento de saida , 2 - caso não tenha
--------------------------------------------------------------------------------------------------------------------------------*/
int temRedirecionamento(char *comandoLido)
{
	if(strchr(comandoLido, '>')!=NULL)
		return REDIRECIONAMENTO_SAIDA;
	
	else if(strchr(comandoLido, '<')!=NULL)
		return REDIRECIONAMENTO_ENTRADA;
		
	else if((strchr(comandoLido, '<')==NULL) && (strchr(comandoLido, '>')==NULL)) //Nao tem redirecionamento. Um comando simples
		return NAO_TEM_REDIRECIONAMENTO;
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Decompor a string lida(comandoLido), nas componentes comando e ficheiro
Recebe: quatro vectores(strings)
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void separarComando(char *comandoLido, char *comando, char *ficheiro, char *redirecionador)
{
	
	char *pComando = (char *)strtok(comandoLido, redirecionador);
	char *pFicheiro = (char *)strtok(NULL, " ");
	
	strcpy(comando, pComando);
	strcpy(ficheiro, pFicheiro);
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Abrir um ficheiro para escrita
Recebe: um vector de char  e uma varivel inteira
Retorna: Um valor inteiro. um número que identifica o ficheiro(descritor)
--------------------------------------------------------------------------------------------------------------------------------*/
int abrirFicheiroEscrita(char *caminhoFicheiro, int tipoAbertura)
{
	int fd;
	switch(tipoAbertura)
	{
		case VAZIO:
			fd = open(caminhoFicheiro, O_WRONLY | O_CREAT | O_TRUNC, 0755);
			break;
		case NAO_VAZIO:
			fd = open(caminhoFicheiro, O_WRONLY | O_APPEND);
			break;
	}
	if(fd<0)
	{
		printf("ERRO AO ABRIR FICHEIRO\n");
		exit(errno);
	}
	return fd;
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Abrir um ficheiro leitura
Recebe: um vector de char
Retorna: Um valor inteiro. um número que identifica o ficheiro(descritor)
--------------------------------------------------------------------------------------------------------------------------------*/
int abrirParaLeitura(char *caminhoFicheiro)
{
	int fd;
	
	fd = open(caminhoFicheiro, O_RDONLY);
	if(fd<0)
	{
		printf("ERRO AO ABRIR FICHEIRO\n");
		exit(errno);
	}
	//printf("ARQUIVO ABERTO\n");
	return fd;
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Fechar o ficheiro
Recebe: Um valor inteiro (Descritor do ficheiro)
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void fecharFicheiro(int descritorFicheiro)
{
	close(descritorFicheiro);
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Escrever um determinado texto no ficheiro
Recebe: um vector de char(texto a ser escrito), e uma variavel inteira( Descritor do ficheiro)
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void escreverNoArquivo(int descritorFicheiro, char *textoComando)
{
	ssize_t bytesEscritos;
	int tamanhoFicheiro;
	char textoReformado[TAMANHO_LINHA];	//variavel para colocar o \n no arquivo
	
	strcpy(textoReformado,textoComando);
	strcat(textoReformado, "\n");
	
	tamanhoFicheiro = strlen(textoReformado);
	
	bytesEscritos = write(descritorFicheiro, textoReformado, tamanhoFicheiro);
	if(bytesEscritos < tamanhoFicheiro)
	{
		printf("ERRO ESCREVENDO NO ARQUIVO\n");
		exit(errno);
	}
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Ler um determindo dado no ficheiro
Recebe: Um valor inteiro (Descritor do ficheiro)
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void lerNoArquivo(int descritorFicheiro)
{
	const int tamanhoFicheiro= 500;
	char textoLido[tamanhoFicheiro];
	ssize_t bytesLidos;
	
	bytesLidos = read(descritorFicheiro, textoLido, tamanhoFicheiro);
	
	if(bytesLidos < 0)
	{
		printf("ERRO AO LER NO ARQUIVO\n");
		exit(errno);
	}
	printf("Lido:\n%s\n", textoLido);
	//printf("\nForam lidos %ld bytes do arquivo", bytesLidos);
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Executar um comando digitado
Recebe: Dois vectores de char(strings) e uma variavel inteira
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void executarComando(char *comando, char *ficheiro,int redirecionamento)
{
	int i=0;
	char *argumentos[TAMANHO_LINHA/5];
	char *pComando;
	int descritorFicheiro;


	switch(redirecionamento)
	{
		case REDIRECIONAMENTO_ENTRADA:
			printf("Redirecionamento de entrada!\n");
			descritorFicheiro = abrirParaLeitura(ficheiro);
			dup2(descritorFicheiro, STDIN_FILENO);
			close(descritorFicheiro);
			break;
		case REDIRECIONAMENTO_SAIDA:
			printf("ENtrou no de saida!");
			descritorFicheiro = abrirFicheiroEscrita(ficheiro, VAZIO);
			dup2(descritorFicheiro, STDOUT_FILENO);
			close(descritorFicheiro);
			break;
		case NAO_TEM_REDIRECIONAMENTO:
			printf("Sem redirecionamento\n");
			break;
	}
	
		
	pComando =(char*)strtok(comando, " ");
	// Alocando espaços para cada argumento dos comandos 15 caracteres p/ cada argumento
	argumentos[i] = (char*) malloc(sizeof(char) *TAMANHO_ARGUMENTOS); 
	strcpy(argumentos[i], pComando);
	i++;
	while((pComando = (char*)strtok(NULL, " "))!=NULL)
	{
		argumentos[i] = (char*) malloc(sizeof(char) *TAMANHO_ARGUMENTOS);
		strcpy(argumentos[i], pComando);
		i++;
	}
	argumentos[i] = (char*) malloc(sizeof(char) *TAMANHO_ARGUMENTOS);
	argumentos[i] = NULL;
		
	execvp(argumentos[0], argumentos);
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Criar um processo
Recebe: Dois vectores de char(strings),uma enumeração e uma variavel inteira
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/	
void criarProcesso(char *comando, char *ficheiro,TPlano plano, int redirecionamento)
{
	int statusEspera;
	int novoProcesso = fork();
	
	if(novoProcesso < 0)//Erro ao criar processo!
	{
		printf("ERRO: Nao foi possivel criar processo para comando!\n");
		return;
	}
	else if(novoProcesso > 0)//Processo Pai
	{
		if(plano == FOREGROUND)
		{
			//printf("Processo pai ativo e entrando em espera pelo Filho!\n");
			wait(&statusEspera);
			if(WIFEXITED(statusEspera))
				printf("\n\nComando terminado com sucesso[%d]!\n", WEXITSTATUS(statusEspera));
			else if(WIFSIGNALED(statusEspera))
				printf("\n\nComando cancelado![%d]\n", WTERMSIG(statusEspera));
			else if(WIFSTOPPED(statusEspera))
				printf("\n\nComando cancelado com sinal de [STOP]\n");
		}
		else
		{
			printf("\n\nRodar comando em segundo plano!\n");			//flagBackground =TRUE;
			ficha.processosPausados[ficha.nElem++]= novoProcesso;
			kill(novoProcesso, SIGSTOP); 
		}
		
	}
	else if(novoProcesso ==0)//Processo filho
	{
		if(isComando(comando, "fg") || isComando(comando, "fg "))
		{	
			if(ficha.nElem>0)
			{ 
				//kill(ficha.processosPausados[ficha.nElem], SIGCONT);
				kill(ficha.processosPausados[ficha.nElem-1], SIGCONT);
				ficha.nElem--;
			}else
				printf("msh nao possui programas em background\n");
			exit(0);
		}
		executarComando(comando, ficheiro, redirecionamento);	
		exit(0);
	}
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Fornecer o indice da posição da última ocorrencia do caracter na string
Recebe: um vector de char(comando) e um caracter
Retorna: O indice da ultima ocorrencia do catecter na string ou -1 caso não tenha na string
--------------------------------------------------------------------------------------------------------------------------------*/
int ultimaOcorrencia(char *comando, char caracter)
{
	int indice = -1;
	for(int i=0; comando[i]!='\0'; i++)
		if(comando[i]==caracter)
			indice = i;
	return indice;
}
/*------------------------------------------------------------------------------------------------------------------------------
Objectivo: Imprimir uma mensagem caso o usuário digite ctrl c
Recebe: um variavel inteira
Retorna: Nada
--------------------------------------------------------------------------------------------------------------------------------*/
void evento(int signal)
{
	printf("\nVoce digitou ctrl+c\n");
}
/*------------------------------------------------------------------------------------------------------------------------------
Menu Principal do programa
--------------------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
		
	signal(SIGINT,evento);
	ficha.nElem=0;	
	char comandoLido[TAMANHO_LINHA];
	char comandoAnterior[TAMANHO_LINHA];
	char comando[TAMANHO_LINHA];
	char ficheiro[TAMANHO_LINHA];
	int descritorFicheiro;
	TPlano planoComando=FOREGROUND;
	
	
	limparTela();
	printf("---------------------------BEM VINDO MEU BASH---------------------------\n");
	
	descritorFicheiro = abrirFicheiroEscrita("msh.hist", VAZIO);
	fecharFicheiro(descritorFicheiro);
	printf("FICHEIRO CRIADO\n");
	
	lerComando(comandoLido);
	
	if(isComando(comandoLido, "exit")==TRUE) return 0;
	
	do{
		
		if(isComandoVazio(comandoLido))
			printf("Comando Vazio!\n");
		else/* if(!isComandoVazio(comandoLido))*/
		{
			if(isBackground(comandoLido))
			{
				//Rodar em Background
				planoComando=BACKGROUND;
				comandoLido[ultimaOcorrencia(comandoLido, '&')] = '\0';
			}
			
			int resultadoRedirecionamento = temRedirecionamento(comandoLido);
			
			switch(resultadoRedirecionamento)
			{
				case NAO_TEM_REDIRECIONAMENTO:
					strcpy(comando, comandoLido); //Comando simples sem redirecionamento!
					break;
				case REDIRECIONAMENTO_ENTRADA:
					separarComando(comandoLido, comando, ficheiro, "<");
					break;
				case REDIRECIONAMENTO_SAIDA:
					separarComando(comandoLido, comando, ficheiro, ">");
					break;
			}
					//Comando simples sem redirecionamento!
					//printf("Nao tem redirecionamento\n");
					if(isComando(comando, "historia")==TRUE)
					{
						printf("|-------------Historia-----------------|\n");
						descritorFicheiro = abrirParaLeitura("msh.hist");
						lerNoArquivo(descritorFicheiro);
						fecharFicheiro(descritorFicheiro);
					}
					else if(isComando(comando, "!!")==TRUE)
					{
						printf("|--------------Ultimo Comando--------------|\n");
						strcpy(comandoLido, comandoAnterior);
						continue;
					}
					else
					{
						criarProcesso(comando, ficheiro, planoComando, resultadoRedirecionamento);	
					}
			
					descritorFicheiro = abrirFicheiroEscrita("msh.hist", NAO_VAZIO);
					escreverNoArquivo(descritorFicheiro,comandoLido);
					fecharFicheiro(descritorFicheiro);
					
					
		}
		
		planoComando=FOREGROUND;
		guardarUltimoComando(comandoAnterior,comandoLido);
		apagarComando(comandoLido);
		apagarComando(comando);
		apagarComando(ficheiro);
		lerComando(comandoLido);

		
	}while(!isComando(comandoLido, "exit")==TRUE);
	
	return 0;
}
