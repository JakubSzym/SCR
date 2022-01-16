#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define HASHED_PASSWD_SIZE 33
#define PASSWD_SIZE        20
#define MAX_MAIL_SIZE      30
#define MAX_LOGIN_SIZE     20
#define AMOUNT_OF_USERS    11
#define DICT_SIZE          14
#define MAX_WORD_SIZE      32
#define MAXIMAL            50


typedef enum
{
  READY,
  BUSY
} ThreadStatus;

typedef enum
{
  EXPLICIT,
  IMPLICIT
} PasswdStatus;

typedef enum
{
  HIDE,
  SHOW
} DisplayPermission;

typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  ThreadStatus status;
} ThreadManager;

typedef struct
{
  int id;
  char hashedPasswd[HASHED_PASSWD_SIZE];
  char passwd[PASSWD_SIZE];
  char mail[MAX_MAIL_SIZE];
  char login[MAX_LOGIN_SIZE];
  PasswdStatus passwd_status;
  DisplayPermission permission;
} User;

ThreadManager manager_basic     = { PTHREAD_MUTEX_INITIALIZER,
                                    PTHREAD_COND_INITIALIZER,
				                            BUSY };
ThreadManager manager_prefixes  = { PTHREAD_MUTEX_INITIALIZER,
                                    PTHREAD_COND_INITIALIZER,
                                    BUSY };
ThreadManager manager_postfixes = { PTHREAD_MUTEX_INITIALIZER,
                                    PTHREAD_COND_INITIALIZER,
				                            BUSY };
ThreadManager manager_both      = { PTHREAD_MUTEX_INITIALIZER,
                                    PTHREAD_COND_INITIALIZER,
                                    BUSY };
  
User users[AMOUNT_OF_USERS];
char dictionary[DICT_SIZE][MAX_WORD_SIZE];


void readUser(FILE* file, User *user)
{
    fscanf(file, "%d", &(user -> id));
    fscanf(file, "%s", user -> hashedPasswd);
    fscanf(file, "%s", user -> mail);
    fscanf(file, "%s", user -> login);
    user -> passwd_status = IMPLICIT;
    user -> permission = SHOW;
}

void readDictionary(const char* filename)
{
    FILE* file = fopen (filename, "r");
    int i = 0;
    while (fscanf(file, "%s", dictionary[i]) != EOF)
    {
      i++;
    }
}

void bytes2md5(const char *data, int len, char *md5buf)
{
	  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
	  const EVP_MD *md = EVP_md5();
	  unsigned char md_value[EVP_MAX_MD_SIZE];
	  unsigned int md_len, i;
	  EVP_DigestInit_ex(mdctx, md, NULL);
	  EVP_DigestUpdate(mdctx, data, len);
	  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
	  EVP_MD_CTX_free(mdctx);
	  for (i = 0; i < md_len; i++)
	  {
		snprintf(&(md5buf[i * 2]), 16 * 2, "%02x", md_value[i]);
	  }
}

void showUser(User *user)
{
    printf("%d %s %s %s\n", user -> id, user -> passwd, user -> mail, user -> login);
}

void *producer_basic(void *arg)
{
  for (int i = 0; i < DICT_SIZE; i++)
  {
    pthread_mutex_lock(&manager_basic.mutex);
    char md5[HASHED_PASSWD_SIZE];
    bytes2md5(dictionary[i], strlen (dictionary[i]), md5);
    for (int j = 0; j < AMOUNT_OF_USERS; j++)
	  {
      if (strcmp(md5, users[j].hashedPasswd) == 0)
	    {
        strcpy(users[j].passwd,dictionary[i]);
		    users[j].passwd_status = EXPLICIT;
      }
    }
	  manager_basic.status = READY;
    pthread_cond_signal(&manager_basic.condvar);
    pthread_mutex_unlock(&manager_basic.mutex);
  }
  pthread_exit(NULL);
}

void *producer_prefixes (void *arg)
{
  for (int i = 0; i < DICT_SIZE; i++)
  {
    pthread_mutex_lock (&manager_prefixes.mutex);
    int current = 0, max = 33;
    char str[PASSWD_SIZE];
    char md5[HASHED_PASSWD_SIZE];
    while (current != max)
	  {
	    sprintf (str, "%d", current);
	    current++;
	    strcat (str, dictionary[i]);
	    bytes2md5 (str, strlen (str), md5);
	    for (int j = 0; j < AMOUNT_OF_USERS; j++)
	    {
	      if (strcmp (md5, users[j].hashedPasswd) == 0)
		    {
		      strcpy (users[j].passwd, str);
		      users[j].passwd_status = EXPLICIT;
		    }
	    }
	  }
    manager_prefixes.status = READY;
    pthread_cond_signal (&manager_prefixes.condvar);
    pthread_mutex_unlock (&manager_prefixes.mutex);
  }
  pthread_exit (NULL);
}

void *producer_postfixes (void *arg)
{
  for (int i = 0; i < DICT_SIZE; i++)
  {
    pthread_mutex_lock (&manager_postfixes.mutex);
    char str[PASSWD_SIZE], res[PASSWD_SIZE];
    char md5[HASHED_PASSWD_SIZE];
    for (int current = 0; current <= MAXIMAL; current++)
	  {
	    sprintf (str, "%d", current);
	    current++;
	    strcpy (res, dictionary[i]);
	    strcat (res, str);
	    bytes2md5 (res, strlen (res), md5);
	    for (int j = 0; j < AMOUNT_OF_USERS; j++)
	    {
	      if (strcmp (md5, users[j].hashedPasswd) == 0)
		    {
		      strcpy (users[j].passwd, res);
		      users[j].passwd_status = EXPLICIT;
		    }
	    }
	  }
    manager_postfixes.status = READY;
    pthread_cond_signal (&manager_postfixes.condvar);
    pthread_mutex_unlock (&manager_postfixes.mutex);
  }
  pthread_exit (NULL);
}

void *producer_both (void *arg)
{
  for (int i = 0; i < DICT_SIZE; i++)
  {
    pthread_mutex_lock (&manager_both.mutex);
    char res[PASSWD_SIZE], str_post[PASSWD_SIZE], res_post[PASSWD_SIZE];
    char md5[HASHED_PASSWD_SIZE];
    for (int prefix = 0; prefix <= MAXIMAL; prefix++)
    {
	    for (int postfix = 0; postfix <= MAXIMAL; postfix++)
	    {
	      sprintf (res, "%d", prefix);
	      sprintf (str_post, "%d", postfix);
	      strcpy (res_post, dictionary[i]);
	      strcat (res_post, str_post);
	      strcat (res, res_post);
	      bytes2md5 (res, strlen (res), md5);
	      for (int j = 0; j < AMOUNT_OF_USERS; j++)
		    {
		      if (strcmp (md5, users[j].hashedPasswd) == 0)
		      {
		        strcpy (users[j].passwd, res);
		        users[j].passwd_status = EXPLICIT;
		      }
		    }
	    }
	  }
    manager_both.status = READY;
    pthread_cond_signal (&manager_both.condvar);
    pthread_mutex_unlock (&manager_both.mutex);
  }
  pthread_exit (NULL);
}

void *consumer (void *arg)
{
    for (int i = 0; i < DICT_SIZE; i++)
    {
      pthread_mutex_lock (&manager_prefixes.mutex);
      pthread_mutex_lock (&manager_basic.mutex);
      pthread_mutex_lock (&manager_postfixes.mutex);
      pthread_mutex_lock (&manager_both.mutex);
      
      while (manager_basic.status == BUSY &&
	           manager_prefixes.status == BUSY &&
	           manager_postfixes.status == BUSY &&
	           manager_both.status == BUSY)
      {
	      pthread_cond_wait (&manager_basic.condvar, &manager_basic.mutex);
	      pthread_cond_wait (&manager_prefixes.condvar, &manager_prefixes.mutex);
	      pthread_cond_wait (&manager_postfixes.condvar, &manager_postfixes.mutex);
	      pthread_cond_wait (&manager_both.condvar, &manager_both.mutex);
      }
      
      manager_basic.status = BUSY;
      manager_prefixes.status = BUSY;
      manager_prefixes.status = BUSY;
      manager_both.status = BUSY;

      pthread_mutex_unlock (&manager_both.mutex);
      pthread_mutex_unlock (&manager_postfixes.mutex);
      pthread_mutex_unlock (&manager_basic.mutex);
      pthread_mutex_unlock (&manager_prefixes.mutex);
      
      for (int j = 0; j < AMOUNT_OF_USERS; j++)
	    {
	      if (users[j].passwd_status == EXPLICIT &&
	          users[j].permission == SHOW)
	      {
	        showUser (&users[j]);
	        users[j].permission = HIDE;
	      }
	    }
    }
    pthread_exit (NULL);
}

int main ()
{
    pthread_t t_producer_basic,
              t_producer_prefixes,
              t_producer_postfixes,
              t_producer_both,
              t_consumer;
    pthread_attr_t attr;

    pthread_attr_init (&attr);
  
    FILE* file = fopen ("passwords.txt", "r");
    
    if (file == NULL)
    {
      exit(1);
    }
    
    for (int i=0; i < AMOUNT_OF_USERS; i++)
    {
      readUser (file, &(users[i]));
    }
    
    readDictionary ("dictionary.txt");

    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create (&t_producer_basic, &attr, producer_basic, NULL);
    pthread_create (&t_producer_prefixes, &attr, producer_prefixes, NULL);
    pthread_create (&t_producer_postfixes, &attr, producer_postfixes, NULL);
    pthread_create (&t_producer_both, &attr, producer_both, NULL);
    pthread_create (&t_consumer, &attr, consumer, NULL);

    pthread_join (t_producer_basic, NULL);
    pthread_join (t_producer_prefixes, NULL);
    pthread_join (t_producer_postfixes, NULL);
    pthread_join (t_producer_both, NULL);
    pthread_join (t_consumer, NULL);

    pthread_attr_destroy (&attr);
    pthread_exit (NULL);
}
