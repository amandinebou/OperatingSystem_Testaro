#include "testaro.h"

// Numéro de la ligne en cours de lecture
int NUMLIGNE = 0;

// Variable où stocker errno lorsque nécessaire
int ERREUR;

// Variable du délai de garde modifiable par l'utilisateur
int TIMEOUT = 5;

// Variable pour stocker la reception d'un éventuel signal et renvoyer le bon code d'erreur à la fin
int SIGNAL = -4;

/** Fonction principale qui ouvre, lit et ferme le fichier. Renvoie 0 si tout s'est bien passé. 
 * argv ne devrait contenir que le nom du fichier à lire
 */
int main(int argc, char *argv[]) {

	
	// Teste si le fichier est bien passé en argument
	if (argc != 2) {
		fprintf(stderr, "Erreur : il faut passer le seul fichier de description en argument");
		exit(1);
	}

	// On ouvre le fichier avec des droits de lecture
    FILE *fichier = fopen(argv[1], "r");  

	// Code d'erreur si fichier ne peut être lu 
	if (fichier == NULL) {
		ERREUR = errno;
		fprintf(stderr, "Erreur dans la lecture du fichier (n° erreur : %d)\n", ERREUR);
		perror("File");
		exit(1);
	}

	// On change le comportement du processus a la reception d'un signal 
	// On s'occupe de SIGALRM plus tard pour le délai
	// SIGCHLD est ignoré car il sert a alerter le pere que le fils a finit donc sera toujours envoyé
	// SIGKILL et SIGSTOP ne peuvent être modifié
	// Pour renvoyer le code erreur demandé 
	for (int i = 1; i < 32 ; i++ ){ 
		
		if (i != SIGALRM && i != SIGKILL && i != SIGSTOP && i!= SIGCHLD){ 
			struct sigaction nvt,old; 
			memset(&nvt, 0, sizeof(nvt)); 
			nvt.sa_handler = signalreceived_handler;
 			
			 if ((sigaction(i, &nvt, &old)) == -1){ 
				 ERREUR= errno; 
				 fprintf(stderr, "Erreur en modifiant l'action associée à un signal (n° erreur : %d)\n", ERREUR); 
				 perror("Sigaction"); 
				 exit(4); } 
		} 
	}

	char *ligne = (char*)malloc(sizeof(char));
	size_t longueur = 0;
	char *buff = (char*)malloc(sizeof(char));
	char *accumulateurEntree = (char*)malloc(sizeof(char));
	char *accumulateurSortie = (char*)malloc(sizeof(char));
	
	// On vérifie que les variables ont bien pu être initialisées
	if (ligne == NULL || buff == NULL || accumulateurEntree == NULL || accumulateurSortie == NULL){
		ERREUR = errno;
		fprintf(stderr, "Problème dans l'allocation de mémoire (n° erreur : %d)\n", ERREUR);
		perror("Malloc");
		exit(ERREUR+40);
	}


	// Delai de garde
	struct sigaction nvt,old;
	memset(&nvt, 0, sizeof(nvt));
	nvt.sa_handler = handler;
	
	if (sigaction(SIGALRM, &nvt, &old) == -1){
		ERREUR = errno;
		fprintf(stderr, "Problème pour modifier l'action associée au signal alarme (n° erreur : %d)\n", ERREUR);
		perror("Sigaction");
		exit(4);
	}

	//boucle principale de lecture du fichier de description
	while (getline(&ligne, &longueur, fichier) != -1)
		{
			++NUMLIGNE;
			treatLine(ligne, buff, accumulateurEntree, accumulateurSortie);
		}
	
	free(ligne);
	free(buff);
	free(accumulateurEntree);
	free(accumulateurSortie);
	
	if (fclose(fichier) == EOF){
		ERREUR = errno;
		fprintf(stderr, "Erreur a la fermeture du fichier (n° erreur : %d)\n", ERREUR);
		perror("Fclose");
		exit(4);

	}
    return SIGNAL+4;
}

/**
 * fonction de traitement d'une ligne du fichier de description
 * @param ligne ligne à lire
 * @param buff eventuellement ligne précedante à concatener
 * @param accumulateurEntree accumulateur qui stocke les parametres à donner en entrée de la prochaine commande
 * @param accumulateurSortie accumulateur qui stocke les sorties eventuelles attendues par la prochaine commande
 * La fonction ne renvoie rien si tout se passe bien, ou interrompt le processus si erreur de lecture. 
*/
void treatLine(char *ligne, char *buff, char *accumulateurEntree, char *accumulateurSortie) {
	int size = strlen(ligne);

	// Cas d'une ligne blanche
	int i;
	for (i = 0 ; i < size ; i++) {
		if (ligne[i] != ' ') break;
	}
	if (i == size-1) {
		return;
	}

	// Cas '\' en fin de ligne : On ajoute 
	if (ligne[size - 3] != '\\' && ligne[size - 2] == '\\') {
		strncat(buff, ligne, size-2);
		return;
	}

	// Cas '\\' en fin de ligne : on n'ajoute pas la ligne suivante
	if (ligne[size-3] == '\\' && ligne[size-2] == '\\') {
		ligne[size-2] = '\n';
		ligne[size-1] = '\0';
	}

	// On lit la ligne : on fusionne avec le buffer (ligne précédente) qui est non vide si '\' dans la ligne précédente
	if (strlen(buff) > 0) {
		// Concatene la ligne précédente avec la ligne courante
		strcat(buff, ligne);
		// Copie la ligne entière dans la ligne courante
		strcpy(ligne, buff);
		size = strlen(ligne);
	}
	// Vider le buffer
	buff[0] = '\0';

	// Cas d'un commentaire
	if (ligne[0] == '#' && ligne[1] == ' ') {
		return;
	}

	// Cas d'une ligne p
	if (ligne[0] == 'p' && ligne[1] == ' ') {
		printf("%s\n", ligne+2);
		return;
	}

	// Cas d'un changement du timeout
	if (strstr(ligne, "! set timeout") != NULL) {
		TIMEOUT = atoi(ligne+13);
		return;
	}

	
	// Cas d'un '<' en début de ligne
	if (ligne[0] == '<' && ligne[1] == ' ') {
		strncat(accumulateurEntree, ligne+2, size-1);
		return;
	}

	// Cas d'un '>' en début de ligne
	if (ligne[0] == '>' && ligne[1] == ' ') {
		strncat(accumulateurSortie, ligne+2, size-1);
		return;
	}

	// Cas d'un '$' en début de ligne
	if (ligne[0] == '$' && ligne[1] == ' ') {
		char *cmd = (char*)malloc(sizeof(char));
		
		if (cmd == NULL){
			ERREUR = errno;
			fprintf(stderr, "Problème dans l'allocation de mémoire (n° erreur : %d)\n", ERREUR);
			perror("Malloc");
			exit(ERREUR+40);
		}

		strncat(cmd, ligne+2, size-1);
		
		testFunction(accumulateurEntree, accumulateurSortie, cmd);

		accumulateurEntree[0] = '\0';
		accumulateurSortie[0] = '\0';
		cmd[0] = '\0';

		return;
	}

	//Ligne de type inconnu
	fprintf(stderr, "Erreur de syntaxe à la ligne %d: ligne de type inconnu\n", NUMLIGNE);
	exit(1);
}

/** Fonction qui execute une commande
 * @param accumulateurEntree char* contenant l'accumulateur d'entrée ( qui peut être vide )
 * @param accumulateurSortie char* contenant l'accumulateur de Sortie ( qui peut etre vide )
 * @param cmd char* contenant la commande à executer
 * La fonction ne renvoie rien mais interrompt le processus si l'accumulateur de Sortie ne correspond pas a la sortie obtenu.
 */

void testFunction(char* accumulateurEntree, char* accumulateurSortie, char* cmd) {
	
	int tube1[2], tube2[2];
	if ( pipe(tube1) == -1 || pipe(tube2) == -1){
		ERREUR= errno;
		fprintf(stderr, "Erreur dans l'appel systeme pipe (n° erreur : %d)\n", ERREUR);
		perror("Pipe");
		exit(4);
	}
	
	int int_fork, status;

	int taille = strlen(accumulateurEntree);
	
	// Cas cd
	if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
		char *path = (char*)malloc(sizeof(char));
		
		if (path == NULL){
			ERREUR = errno;
			fprintf(stderr, "Problème dans l'allocation de mémoire (n° erreur : %d)\n", ERREUR);
			perror("Malloc");
			exit(ERREUR+40);
		}

		int taillecmd = strlen(cmd);

		if (cmd[taillecmd-1] == '\n'){
			strncat(path, cmd+3, taillecmd-4);
		}
		else{
			strncat(path, cmd+3, taillecmd-3);
		}
		if (chdir(path) <0){
			ERREUR = errno;
			fprintf(stderr, "Erreur à l'appel système cd de la ligne %d (n° erreur : %d)\n", NUMLIGNE, ERREUR);
			perror("cd");
			exit(4);
		}
		free(path);
		return;
	}
	
	// Cas su
	/**
	 * Version de su qui fonctionne en mode root uniquement
	 * 
	if (cmd[0] == 's', cmd[1] == 'u', cmd[2] == ' '){
    
    const char* user = "mettre_nom_user_ici_a_partir_de_cmd";
    struct passwd *password = getpwnam(user);
    if (password == NULL){
        fprintf(stderr, "Cet utilisateur n'existe pas");
        exit(4);
    }
    
	int id = password -> pw_uid ;
    int group_id = password -> pw_gid;
    
    if (setgid(group_id) != 0){
        fprintf(stderr, "setgid() vers %d a échoué \n", group_id);
        perror("setgid");
        exit(1);
    }

    if (setuid(id) != 0){
        fprintf(stderr, "setuid() vers %d a échoué \n", id);
		perror("setuid");
        exit(1);
    }

	}*/

	
	char *message = (char*) malloc(sizeof(char));
	
	if (message == NULL){
			ERREUR = errno;
			fprintf(stderr, "Problème dans l'allocation de mémoire (n° erreur : %d)\n", ERREUR);
			perror("Malloc");
			exit(ERREUR+40);
	}

	
	int_fork =  fork();
	
	switch(int_fork)
	{
		case -1 :
			fprintf(stderr, "Erreur dans la création d'un processus fils \n");
			perror("Fork");
			exit(4);

		case 0 : 
			close(tube1[1]);
			
			dup2(tube1[0], 0);
			dup2(tube2[1], 1);
			
			close(tube1[0]);
			close(tube2[0]);
			close(tube2[1]);
			close(tube1[0]);
			
			execlp("sh", "sh", "-c", cmd, NULL);
			exit(4);

		default :
			
			fflush(stdout);
			if (write(tube1[1], accumulateurEntree, taille) == -1 ){
				ERREUR = errno;
				fprintf(stderr, "Erreur dans l'appel systeme write lors de l'execution de la commande ligne %d (n° erreur : %d)\n", NUMLIGNE, ERREUR);
				perror("Write");
				exit(4);
			}
			
			close(tube1[1]);

			alarm(TIMEOUT);
			wait(&status);
			close(tube2[1]);
			
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0){
				char* recu = (char*) malloc(1024 * sizeof(char));
				int lu;
				while ((lu = read(tube2[0], recu, 1024)) > 0){
					strncat(message, recu, lu);
				}

				if (lu == -1){
					ERREUR = errno;
					fprintf(stderr, "Erreur a l'appel systeme Read lors de l'execution de la commande ligne %d (n° erreur : %d)\n", NUMLIGNE, ERREUR);
					perror("Read");
					exit(4);
				}
				close(tube2[0]);
				printf("Message renvoyé par la commande ligne %d :\n%s \n", NUMLIGNE, message);
			}
			else{
				close(tube2[1]);
				close(tube2[0]);
				fprintf (stderr, "Erreur l'execution de la commande à la ligne %d, code : %d \n", NUMLIGNE, WEXITSTATUS(status));
				exit(WEXITSTATUS(status)+40);
			}
	}
	
	
	if (strlen(accumulateurSortie) > 0 && strcmp(accumulateurSortie, message) != 0){
		printf("La commande ligne %d n'a pas renvoyé le texte attendu\n", NUMLIGNE);
		printf("Message attendu : '%s'\nMessage obtenu : '%s'", accumulateurSortie, message);
		exit(2);
	}

	message[0] = '\0';
	free(message);
	return;
}

/**
 * Fonction handler pour le signal alarme pour gérer le délai de garde/
 * Interrompt le processus en renvoyant un code d'erreur 3.
 * */
void handler() {
	printf("La commande ligne %d n'a pas terminé avant l'expiration du délai de garde de %d secondes\n", NUMLIGNE, TIMEOUT);
	exit(3);
}

/**
 * Fonction handler pour chaque signaux (sauf 4 exceptions) reçu par le programme testaro.
 * @param signo numéro du signal reçu.
 * Modifie la variable globale SIGNAL.
 * */
void signalreceived_handler(int signo){
	SIGNAL = signo;
}
