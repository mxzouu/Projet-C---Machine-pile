#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

int lines_count = 0;// nombre de lignes du fichier contenant le code assembleur
int exa_count=0;// nombre de lignes du fichier héxa

//fonction qui récupère les lignes d'un fichier 
// inspirée d'une technique d'optimisation vue sur internet
char** read_lines_from_file(FILE* file) {
    fseek(file, 0, SEEK_SET);
    char** lines = NULL;// ça renvoie NULL si le fichier est vide
    char  line[100];
    while (fgets(line, 100, file)) {// nous faisons l'hypothèse qu'une ligne ne dépasse pas 100 caractères
         line[strcspn(line, "\n")] = '\0';// on enlève le \n 
        lines = realloc(lines, (lines_count + 2) * sizeof(char*));
        lines[lines_count] = malloc(strlen(line) + 1);
        strcpy(lines[lines_count], line);
        lines_count++;
    }
    return lines;
}
// version similaire à la précédente, nous la recopions juste pour avoir le bon nombre de lignes, bien évidemment il aurait été plus efficace de mettre en argument de la fonction précédente l'adresse d'une variable qui contiendra le nombre de lignes.
char** read_lines_from_file_exa(FILE* file) {
    fseek(file, 0, SEEK_SET);
    if (file==NULL) {printf("fichier exa dans read lines vide \n");return NULL;}
    char** lines = NULL;
    char  line[100];
    while (fgets(line, 100, file)) {
         line[strcspn(line, "\n")] = '\0';// on enlève le \n 
        lines = realloc(lines, (exa_count + 2) * sizeof(char*));
        lines[exa_count] = malloc(strlen(line) + 1);
        strcpy(lines[exa_count], line);
        exa_count++;// seul modification par rapport à la précédente
    }
    return lines;
}
// nous nous servons de codeInstruction principalement pour avoir le code d'une instruction qui n'est rien d'autre que son indice dans ce tableau
const char *CodesInstruction[]={"push","push#","ipush","pop","ipop","dup","op","jmp","jpz","rnd","read","write","call","ret","halt"};
const char *NoArgumentInstruction[]={"ipush","ipop","dup","ret","halt"};// tableau contenant les instructions qui ne prennent pas d'argument
const char *OneArgumentInstruction[]={"push","push#","pop","op","jmp","jpz","rnd","read","write","call"};// instructions qui prennent un seul argument

int SP=0;// pointeur de pile
int PC=0;// pointeur d'instructions
int memory[5000];// mémoire de la machine, une partie représente la pile
///------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction qui renvoie le code en decimal d'une instruction passée en argument

int DecimalCode(char *instruction){
   for(int i=0; i<15; i++){
    if (!(strcmp(CodesInstruction[i],instruction))){
        return i;
    }
   }
   return -100; /// code d'erreur si il a passé une instruction qui n'existe pas
}
///-----------------------------------------------------------------------------------------------------------------------------------------
///fonction qui renvoie sous forme de chaine de caractère la représentation hexa decimale de l entier passé en argument
char *decToExa(int d){
  char *exa_temp=malloc(10);// dans le pire des cas ca serait 8 bits pour la donnee
  sprintf((exa_temp),"%x",d);/// elle écrit d dans le buffer exa_temp
  return exa_temp;
}

///-------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction qui rajoute des zero au code de linstruction pour avoir 2 octets
void addZeroCode(char *exa_temp_code){
    if (strlen(exa_temp_code)>2){ printf("ca depasse 2 pour le code ");return;}
    if (strlen(exa_temp_code)<2){
        char dest[10]="\0";
        strcat(dest,"0");
        strcat(dest,exa_temp_code);
        strcpy(exa_temp_code,dest);
    }

}
///---------------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction qui rajoute des zero a la donnée de linstruction pour avoir 8 octets
void addZeroData(char *exa_temp_data){
   if (strlen(exa_temp_data)>8) {printf(" ca depasse 8 bits");return;}
   if (strlen(exa_temp_data)< 8 ){
        char dest[10];
        while (strlen(exa_temp_data)<8){
            strcpy(dest,"0");
            strcat(dest,exa_temp_data);
            strcpy(exa_temp_data,dest);
        }
   }

}
///----------------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction qui renvoie 0 si l'étiquette n'a pas : à la fin, et renvoie 1 si c'est le cas
int isEtiquette(char *etiq){
  int taille=strlen(etiq);
  if (etiq[taille-1]==':'){
    return 1;
  }
  return 0;
}

///--------------------------------------------------------------------------------------------------------------------------------------------------------
/// renvoie si c'est une instruction valide ou pas

int isCode(char* str){
   for(int i=0; i<15; i++){
    if (!(strcmp(CodesInstruction[i],str))){
        return 1;
    }
   }
   return 0;
}

///--------------------------------------------------------------------------------------------------------------------------------------------------------
/// renvoie un tablezu qui contient les mots qui forment une lignes, les mots sont séparés par des tabulations ou 1 voire plus d'espaces
char ** split(const char * str, const char * delim)
{
  char * s = strdup(str);
  if (strtok(s, " \n\t:") == 0){
    free(s);
    return NULL;}
  int nw = 1;
  while (strtok(NULL, " \n\t:") != 0)
    nw += 1;//nombre de mots
  strcpy(s, str); 
  char ** v = malloc((nw + 1) * sizeof(char *));
  int i=0;
  int longueur=strcspn(s, " \n\t:");// donne la longueur de l plus petite chaine avant \n \t ou :
  v[0] = malloc(sizeof(char)*(longueur+2));// pour enregistrer l'étiquette avec ses deux points
  strcpy(v[0],strtok(s, " \n\t:"));// l'étiquette ne peut se trouver qu'au début sinon on signale une erreur, et si jamais on est sur une étiquette, on rajoute les :
  if(str[longueur]==':'){
     v[0][longueur]=':';
     v[0][longueur+1]='\0';
  }
  for (i = 1; i < nw; ++i)
    v[i] = strdup(strtok(NULL, " \n\t"));
  v[i] = NULL; // marque la fin du tableau, on aurait pu utiliser une structure avec comme champs le tableau et sa longueur.
  free(s);
  return v;
}
///----------------------------------------------------------------------------------------------------------------------------------------------------
/// retourne le nombre de mots d'une ligne
int numberOfWords(const char*str, char **words){
 int nb=0;
 if(words==NULL){return nb;}// ca renvoie 0 dans le cas de \n ou \t car split renvoie null dans ces cas là, ce qui nous permet par la suite de facilement ignorer les lignes qui ne contiennent que des \n ou \t
 for (int i=0; words[i]!=NULL; i++ ){
    nb++;
 }
 return nb;
}
///------------------------------------------------------------------------------------------------------------------------------------------------------
/// renvoie le code hexa d'une donnee
char* dataCode(char* data){
    char *v=decToExa(atoi(data));
    addZeroData(v);
   return v;

}
///----------------------------------------------------------------------------------------------------------------------------------------------
/// renvoie le code d'une instruction
char* instCode(char* inst){
   char *v=decToExa(DecimalCode(inst));
   addZeroCode(v);
   return v;

}
/// -----------------------------------------------------------------------------------------------------------------------------------------------
/// fonction a 1 seul argument ou pas
int hasOneArgument(char* code){
   for(int i=0; i<10; i++){
    if ((strcmp(OneArgumentInstruction[i],code))==0){
        return 1;
    }
   }
   return 0;
}
///--------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction n'a pas d argument
int hasNoArgument(char *code){
 for(int i=0; i<5; i++){
    if ((strcmp(NoArgumentInstruction[i],code))==0){
        return 1;
    }
   }
   return 0;

}
///-------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction is Number qui verifie si la chaine représente un nombre en utilisant isDigit on l'utilise par la suite pour vérifier le type d'une donnée
int isNumber(char* data){
   for (int i=0; i<strlen(data); i++){
    if (!(isdigit(data[i]))&& !( data[i]=='-') && !(data[i]=='+')){return 0;}
   }
    return 1;
}

///----------------------------------------------------------------------------------------------------------------------------------------------------
/// renvoyer un tableau qui contient les étiquettes de la totalité du code, les etiquettes n'ont pas les deux points à la fin on l'utilisera  principalement pour vérifier si une étiquette existe bien
char** etiquettee(FILE *fp, char **lines){
    fseek(fp, 0, SEEK_SET);// on remet le curseur au début du fichier par prévention (après une mauvaise expérience)
    char *line=malloc(sizeof(char)*100);
   char **words=malloc(sizeof(char*)*10);
   char **etiquette=malloc(sizeof(char*)*80);
   int i=0;
   for (int j=0;j<lines_count;j++){
        line=lines[j];
        words=split(line," \t\n");
        if (words!=NULL){
            if (isEtiquette(words[0])){
                    char *word_cpy=malloc(sizeof(char)*80);
                    strcpy(word_cpy,words[0]);// car si nn ils pointent vers le même
                    etiquette[i]=word_cpy;//pour pas modifier words dans la ligne qui suit
                    etiquette[i][strlen(etiquette[i])-1]='\0';
                    i++;

            }
        }
         for (int i=0;words[i]!=NULL;i++){
	free(words[i]);
      }
   }
   etiquette[i]=NULL;// pour le reconnaitre a la fin
   free(words);
   return etiquette;
}

///--------------------------------------------------------
/// fonction qui renvoie 1 si l étiquette respecte la syntaxe demandée, 0 si nn
int correctSyntax(char *str){
    if (isalpha(str[0])==0 ){//toujours commencer par un caractère alphabétique
        return 0;
    }
    for (int i=1; i<strlen(str)-2;i++){// je vais pas jusqu'à la fin car dans tous les cas si a la fin il met pas : ça ne sera pas une etiquette et cette fonction est appellée une fois la verification d'étiquette faite
        if (isalnum(str[i])==0){
            return 0;
        }
    }
    return 1;
}

///----------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction qui renvoie 0 si une etiquette est dupliquée, 1 si non
int dupEtiq(char *str, char **etiq){
    // aucune verification n'est faite encore une fois car elles seront faites avant que la fonction soit appellée
    char *str_cpy=malloc(sizeof(char)*strlen(str)+1);
    strcpy(str_cpy,str);
    for(int i=0;str_cpy[i]!='\0';i++){
        if (str_cpy[i]==':'){
            str_cpy[i]='\0';
            break;
        }
    }
    int cpt=0;
    for (int i=0; etiq[i]!=NULL; i++){
        if (strcmp(etiq[i],str_cpy)==0){
            cpt++;
        }
    }
    if (cpt>1){
        free(str_cpy);
        return 0;
    }
    free(str_cpy);
    return 1;
}


/// fonction qui traite le cas des instructions qui ont des adresses à savoir jmp jpz et call, elle renvoie 1 si adr respecte ce qu'il faut, 0 sinn
int adrValable(char *adr, int line,FILE *fp){
    fseek(fp, 0, SEEK_SET);
    if (!(isNumber(adr))){
            return 0;
        }
    else{
        if((atoi(adr)+line)<0 || (atoi(adr)+line)>lines_count){
            return 0;
        }
    }


    return 1;

}

/// fonction qui teste si il y a une erreur ou pas dans le fichier, elle renvoie 0 dès quelle trouve une erreur mais elle parcourt quand même tout et elle print  chaque erreur
int compiled(FILE* fp,char **lines, char **etiquette){
   fseek(fp, 0, SEEK_SET);
   char *line=malloc(sizeof(char)*80);
   char *word_cpy=malloc(sizeof(char)*80);
   char **words=malloc(sizeof(char*)*100);
   if(lines_count==0){printf("le fichier est vide \n"); return 0;}
   int nb_words=0;
   int cpt=0;
   int bool=1; // 1 si c compilé 0 sinon
   for (int j=0;j<lines_count;j++){
        line=lines[j];
       // printf("%d %s \n",j,lines[j]);
        words=split(line," \t\n");
        nb_words=numberOfWords(line,words);
        if (nb_words==1){
            if (isEtiquette(words[0])){
                if(correctSyntax(words[0])&& dupEtiq(words[0],etiquette)){
                    printf("ligne : %d : mettez quelque chose apres l etiquette %s  \n",j+1,words[0]);
                    bool=0;
                }
                else{
                    if (!(correctSyntax(words[0]))){
                        printf("ligne : %d : etiquette %s ne respectant pas la syntaxe des etiquettes \n ",j+1,words[0]);
                        bool=0;
                    }
                    else {
                        printf("ligne : %d : etiquette %s dupliquee \n",j+1,words[0]);
                        bool=0;
                    }
                }
            }
            else {
                if(!(isCode(words[0]))){
                    printf(" ligne  %d : instruction inconnue %s \n",j+1,words[0]);
                    bool=0;
                }
                else{
                    if (hasOneArgument(words[0])){
                        printf(" ligne %d : il manque un argument a l instruction %s \n",j+1,words[0]);
                        bool=0;
                    }
                }
            }
        }
        if(nb_words==2){
            if (isEtiquette(words[0])){
                if(correctSyntax(words[0])&& dupEtiq(words[0],etiquette)){
                    if (!(isCode(words[1]))){
                        printf("ligne %d : une instruction correcte est attendue a la place de : %s \n",j+1,words[1]);
                        bool=0;
                        }
                    else {
                        if (hasOneArgument(words[1])){
                            printf("ligne %d : il manque un argument a l instruction : %s \n ",j+1,words[1]);
                            bool=0;
                        }
                    }
                }
                else{
                     if (!(correctSyntax(words[0]))){
                        printf("ligne : %d : etiquette %s ne respactant pas la syntaxe des etiquettes \n ",j+1,words[0]);
                        bool=0;
                    }
                    else {
                        printf("ligne : %d : etiquette %s dupliquee \n",j+1,words[0]);
                        bool=0;
                    }
                }
            }
            else {
                if (!(isCode(words[0]))){
                    printf("ligne %d : %s inconnu \n",j+1,words[0]);
                    bool=0;
                }
                else{
                    if (hasNoArgument(words[0])){
                        printf("ligne %d : trop d'arguments donnes a : %s \n",j+1,words[0]);
                        bool=0;
                    }
                    else{
                        if ((strcmp(words[0],"jmp"))&& (strcmp(words[0],"jpz")) && (strcmp(words[0],"call"))){
                            if (strcmp("op",words[0])==0){
                                if ((isNumber(words[1]))==0 || atoi(words[1])<0 || atoi(words[1])>15 ){
                                    printf(" ligne : %d : mauvais argument  a l instruction %s \n ",j+1,words[0]);
                                    bool=0;
                                }
                            }
                            if (strcmp("write",words[0])==0 || strcmp("read",words[0])==0 || strcmp("pop",words[0])==0 || strcmp("push",words[0])==0 ){
                                if (isNumber(words[1])==0 || atoi(words[1])>4999 || atoi(words[1])<0){
                                    printf("ligne : %d : mauvais argument pour l instruction %s , adresse introuvable en memoire \n",j+1,words[0]);
                                    bool=0;
                                }
                            }
                            if(strcmp("push#",words[0])==0 || strcmp("rnd",words[0])==0 ){
                                if(isNumber(words[1])==0 || atoi(words[1])> 2147483647 || atoi(words[1])<-2147483648){
                                    printf("ligne: %d : depassement de capacite a l argument de l instruction %s \n",j+1,words[0]);
                                    bool=0;
                                }
                            }
                        }
                        else {
                            if(isNumber(words[1])){
                                if (!(adrValable(words[1],j-cpt,fp))){
                                    printf(" ligne %d : l argument mis a %s est invalide, adresse introuvable \n",j+1,words[0]);
                                    bool=0;
                                }
                            }
                            else {
                                int b=0;
                                for (int v=0;etiquette[v]!=NULL;v++){
                                    //printf("%s \n",etiquette[0]);
                                    if ((strcmp(etiquette[v],words[1]))==0){
                                        b=1;
                                    }
                                }
                                if (b==0){
                                    printf("ligne %d : inconnu : %s \n",j+1,words[1]);
                                    bool=0;
                                }
                            }
                        }
                    }
                }
            }
        }
        if(nb_words==3){
            if (!(isEtiquette(words[0]))){
                printf("ligne %d: syntaxe invalide vous avez peut etre oublie de mettre : a la fin de %s: \n",j+1,words[0]);
                bool=0;
            }
            else{
                if(correctSyntax(words[0])&& dupEtiq(words[0],etiquette)){
                    if (!(isCode(words[1]))){
                        printf("ligne %d : %s inconnu \n",j,words[1]);
                        bool=0;
                    }
                    else{
                        if (hasNoArgument(words[1])){
                            printf("ligne %d : beaucoup d'arguments donnes : %s \n",j,words[1]);
                            bool=0;
                        }
                        else{
                            if ((strcmp(words[1],"jmp"))&& (strcmp(words[1],"jpz")) && (strcmp(words[1],"call"))){
                                if (strcmp("op",words[1])==0){
                                    if ((isNumber(words[2]))==0 || atoi(words[2])<0 || atoi(words[2])>15 ){
                                        printf(" ligne : %d : mauvais argument  a l instruction %s \n ",j,words[0]);
                                        bool=0;
                                    }
                                }
                                if (strcmp("write",words[1])==0 || strcmp("read",words[1])==0 || strcmp("pop",words[1])==0 || strcmp("push",words[1])==0 ){
                                    if (isNumber(words[2])==0 || atoi(words[2])>4999 || atoi(words[2])<0){
                                        printf("ligne : %d : mauvais argument pour l instruction %s , adresse introuvable en memoire ou type incompatible \n",j,words[1]);
                                        bool=0;
                                    }
                                }
                                if(strcmp("push#",words[1])==0 || strcmp("rnd",words[1])==0 ){
                                    if(isNumber(words[2])==0 || atoi(words[2])> 2147483647 || atoi(words[2])<-2147483648){
                                        printf("ligne: %d : depassement de capacite a l argument de l instruction %s \n",j,words[1]);
                                        bool=0;
                                    }
                                }
                            }
                          else {
                                if(isNumber(words[2])){
                                    if (!(adrValable(words[2],j-cpt,fp))){
                                        printf(" ligne %d : l argument mis a %s est invalide, adresse introuvable \n",j,words[0]);
                                        bool=0;
                                    }
                                }
                                else {
                                    int b=0;
                                    for (int v=0;etiquette[v]!=NULL;v++){
                                        if ((strcmp(etiquette[v],words[2]))==0){
                                            b=1;
                                        }
                                    }
                                    if (b==0){
                                        printf("ligne %d : inconnu : %s \n",j,words[2]);
                                        bool=0;
                                    }
                                }
                            }
                        }
                    }
                }
                else{
                    if (!(correctSyntax(words[0]))){
                        printf("ligne : %d : etiquette %s ne respactant pas la syntaxe des etiquettes \n ",j,words[0]);
                        bool=0;
                    }
                    else {
                        printf("ligne : %d : etiquette %s dupliquee \n",j,words[0]);
                        bool=0;
                    }
                }
            }
        }
        if (nb_words>3){

            printf("ligne %d : syntaxe invalide \n",j);
            bool=0;
        }
        if(nb_words==0){
            cpt++;
            continue;
        }
	 for (int i=0;words[i]!=NULL;i++){
	free(words[i]);
      }
   }
   free(word_cpy);
   free(words);
   return bool;
}

///-----------------------------------------------------------------------------------------------------------------------------------------------------
/// fonction qui retourne l'adresse d une etiquette dans un fichier, les adresses commencent par 0
/// elle sera utilisée apres avoir compilé, donc pas la peine de reverifier si il y a l'étiquette dans le fichier
int adressOfEtiquette(FILE* fp,char *etiq, char **lines){
    fseek(fp, 0, SEEK_SET);
    char* line=malloc(sizeof(char)*80);
    char** words=malloc(sizeof(char*)*10);
    int cpt=0;// calcul du nombre de lignes vides
    for(int i=0; i<lines_count; i++){
        line=lines[i];
        words=split(line," \t\n");
        if (numberOfWords(line,words)==0){
            cpt++;
            continue;
        }
        if(strcmp(words[0],etiq)==0){
             for (int i=0;words[i]!=NULL;i++){
	     free(words[i]);
             }
             free(words);
            return i-cpt;// si jamais y a une ligne vide, je ne la prends pas en compte dans le calcul.
        }
         for (int i=0;words[i]!=NULL;i++){
	free(words[i]);
      }
    }
    free(words);
    return -1; // normalement ca ne risque pas d arriver...
}


/// fonction qui renvoie le nom du fichier hexa a partir d'un fichier assembleur
char* fichierExa(FILE* fp, char** lines, char **etiquette ){
    fseek(fp, 0, SEEK_SET);
    int check=compiled(fp,lines,etiquette);
    int cpt=0;
    if (check==0){
        printf ("non compile, veuillez corriger les erreurs si vs voulez que je genere le fichier exa \n");
        return NULL;
    }
    char *line =malloc(sizeof(char)*80);
    char **words=malloc(sizeof(char*)*10);
    char* exaCode=malloc(sizeof(char)*10);
    char* et=malloc(sizeof(char)*50);
    int nb_words=0;
    FILE* exa=fopen("exa.txt","w");
    for(int i=0;i<lines_count;i++){
        line=lines[i];
        words=split(line," \t\n");
        nb_words=numberOfWords(line,words);
        if (nb_words==0){continue;}
        cpt++;
        for(int j=0;j<nb_words;j++){
            if (isEtiquette(words[j])){
                continue;
            }
            if (isCode(words[j])){
                //exaCode=instCode(words[j]);
                fprintf(exa,"%s",instCode(words[j]));
                if (hasOneArgument(words[j])){
                    fprintf(exa,"%c",' ');
                }
                else{
                    fprintf(exa,"%c",' ');
                    fprintf(exa,"%s","00000000");
                    fprintf(exa,"\n");
                }
		free(instCode(words[j]));

            }
           if (isNumber(words[j])){
                //exaCode=dataCode(words[j]);
                fprintf(exa,"%s",dataCode(words[j]));
                fprintf(exa,"\n");
		free(dataCode(words[j]));
            }
            if (isCode(words[j])==0 && isNumber(words[j])==0 && isEtiquette(words[0])==0){
                strcpy(et,words[j]);
                strcat(et,":");
                exaCode=decToExa(adressOfEtiquette(fp,et,lines)-cpt);
                addZeroData(exaCode);
                fprintf(exa,"%s",exaCode);
                fprintf(exa,"\n");

            }
        }
        for (int i=0;words[i]!=NULL;i++){
	free(words[i]);
      }
    }

    free(words);
    free(exaCode);
    free(et);
    fclose(exa);
    return "exa.txt";
}

/// PARTIE 2-----------------------------------------------------------------------------------------------------------------------------
/// fonction qui convertit de l hexa en decimal
int hexaTodec(char *str){
    int dec;
    sscanf(str,"%x",&dec);
    return dec;
}



///--------------------------------------------------------------------------------------------------------------------------------------
/// fonctions push....

void push(int x){
    memory[SP++]=memory[x];
    PC++;
}

void push2(int i){
    memory[SP++]=i;
    PC++;
}

void ipush(){
  int x=memory[SP-1];
  memory[SP-1]=memory[x];
  PC++;

}

void pop(int x){
    SP--;
    memory[x]=memory[SP];
    PC++;
}


void ipop(){
    memory[SP-1]=memory[SP-2];
    SP=SP-2;
    PC++;
}

void dup(){
    memory[SP]=memory[SP-1];
    SP++;
    PC++;
}

void jmp(int adr){
    //printf("l adresse du jump %d \n", adr);
    PC+=adr;
    PC++;
}

void jpz(int adr){
    SP--;
    //printf(" adresse %d \n", adr);
    int x=memory[SP];
    if (x==0){
        PC+=adr;
    }
    PC++;

}

void rnd(int x){
    memory[SP++]=rand()%x;
    PC++;
}

void read (int x){
    printf(" entrez la valeur (attention: garbage in garbage out)  :  ");
    scanf("%d",&memory[x]);
    PC++;
}

void write (int x){
    printf("%d \n",memory[x]);
    PC++;
}

void call (int adr){
    memory[SP++]=PC+1;
    PC+=adr;
    PC++;

}

void ret(){
    PC=memory[--SP];
}

void halt(){
    PC=-1;// c'est une condition que j'utilise par la suite pour terminer un programme quand on voit halt
}

/// les fonctions op
void op(int i){
   switch (i){
        case 0: {
            SP--;
            memory[SP-1]=memory[SP-1]+memory[SP];
            break;
        }
        case 1:{
            SP--;
            memory[SP-1]=memory[SP-1]-memory[SP];
            break;
        }
        case 2:{
            SP--;
            memory[SP-1]=memory[SP-1]*memory[SP];
            break;
        }
        case 3:{
            SP--;
            memory[SP-1]=memory[SP-1]/memory[SP];
            break;
        }
        case 4:{
            SP--;
            memory[SP-1]=memory[SP-1]%memory[SP];
            break;
        }
        case 5:{
            memory[SP-1]=(-memory[SP-1]);
            break;
        }
        case 6:{
            SP--;
            if (memory[SP-1]==memory[SP]){
                memory[SP-1]=0;
            }
            else{
                memory[SP-1]=1;
            }
            break;
        }
        case 7:{
            SP--;
            if (memory[SP-1]!=memory[SP]){
                memory[SP-1]=0;
            }
            else{
                memory[SP-1]=1;
            }
            break;
        }
        case 8:{
            SP--;
            if (memory[SP-1]>memory[SP]){
                memory[SP-1]=0;
            }
            else{
                memory[SP-1]=1;
            }
            break;
        }
        case 9:{
            SP--;
            if (memory[SP-1]>=memory[SP]){
                memory[SP-1]=0;
            }
            else{
                memory[SP-1]=1;
            }
            break;
        }
        case 10:{
            SP--;
            if (memory[SP-1]<memory[SP]){
                memory[SP-1]=0;
            }
            else{
                memory[SP-1]=1;
            }
            break;
        }
        case 11:{
            SP--;
            if (memory[SP-1]<=memory[SP]){
                memory[SP-1]=0;
            }
            else{
                memory[SP-1]=1;
            }
            break;
        }
        case 12:{
            SP--;
            memory[SP-1]=memory[SP-1]&memory[SP];
            break;
        }
        case 13:{
            SP--;
            memory[SP-1]=memory[SP-1]|memory[SP];
            break;
        }
        case 14:{
            SP--;
            memory[SP-1]=memory[SP-1]^memory[SP];
            break;
        }
        case 15:{
            memory[SP-1]=~memory[SP-1];
            break;
        }
        default : perror("pas les bons case pour op \n");
   }
   PC++;
}

///-----------------------------------------------------------------
/// fonction qui execute une ligne
void executeLine(char* line){
    char **words=malloc(sizeof(char*)*3);
    int code,argument;
    words=split(line," ");
    code=hexaTodec(words[0]);
    argument=hexaTodec(words[1]);
    switch (code){
            case 0: {push(argument);break;}
            case 1: {push2(argument);break;}
            case 2: {ipush();break;}
            case 3: {pop(argument);break;}
            case 4: {ipop();break;}
            case 5: {dup();break;}
            case 6: {op(argument);break;}
            case 7: {jmp(argument);break;}
            case 8: {jpz(argument);break;}
            case 9: {rnd(argument);break;}
            case 10: {read (argument);break;}
            case 11: {write(argument);break;}
            case 12: {call(argument);break;}
            case 13: {ret();break;}
            case 14: {halt();break;}
            default : perror("le code de l instruction n'est pas connu \n");
        }
    for (int i=0;words[i]!=NULL;i++){
	free(words[i]);
    }
    free(words);
}

///--------------------------------------------------------------
/// fonction qui fait l'éxécution à partir du fichier héxa
void execution(FILE *fichier_exa, char **lines){
    fseek(fichier_exa, 0, SEEK_SET);
    PC=0; SP=0;
    while(PC!=-1 && lines[PC]!=NULL){
         executeLine(lines[PC]);
         
    }
 }


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("l'éxécutable : %s ne prend en argument qu'un seul fichier\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Echec à l'ouverture du fichier\n");
        return 1;
    }

    char** lines = read_lines_from_file(file);
    if (lines ==NULL) {printf("fichier vide \n");return 0;}
    char **etiq=etiquettee(file,lines);
    FILE *fic_exa=fopen(fichierExa(file,lines,etiq),"r");
    if (fic_exa==NULL) {return 0;}
    char **exaLines=read_lines_from_file_exa(fic_exa);
    if(exaLines==NULL) {printf ("fichier Héxa vide \n"); return 0;}// nous avons fait le choix de ne pas générer d'erreur dans le cas d'un fichier vide mais de générer un fichier Héxa vide.
    exaLines[exa_count]=NULL;// marque de fin
    execution(fic_exa,exaLines);
    
     for (int i=0; etiq[i]!=NULL;i++){
	free(etiq[i]);
}
 
    for (int i=0;i<lines_count;i++){
	free(lines[i]);
}
   
     for (int i=0;i<exa_count;i++){
	free(exaLines[i]);
}
    
    free(etiq);
    free(lines);
    free(exaLines);
    fclose(file);
    fclose(fic_exa);

    return 0;
}

















