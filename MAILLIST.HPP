
/* **********************************************************************
   * MailList.HPP                                                       *
   *                                                                    *
   * Copyright by Fredric L. Rice, October 1994.                        *
   *                                                                    *
   * The Skeptic Tank, 1:102/890.0, (818) 335-9601.                     *
   *                                                                    *
   ********************************************************************** */

/* **********************************************************************
   * Define macros                                                      *
   *                                                                    *
   ********************************************************************** */

#define skipspace(s)    while (isspace(*s))     ++s
#define TRUE            1
#define FALSE           0
#define USHORT          unsigned short
#define BOOL            unsigned char
#define The_Version     "1.200"

/* **********************************************************************
   * The class we are going to use.                                     *
   *                                                                    *
   ********************************************************************** */

    struct Name_List {
        char *name;                     /* The person's name            */
        char *address;                  /* The person's mailing address */
        struct Name_List *next;         /* A pointer to the next one.   */
    } ;                                 /* No pointers here.            */

    struct Group_List {
        char *group_name;               /* The name of the mail group   */
        char *mail_directory;           /* Where is outbound mail sent? */
        char *from_name;                /* Who is the message from?     */
        char *origin;                   /* The group origin line to use */
        USHORT name_count;              /* How many here?               */
	struct Name_List *nl_first;     /* A pointer to names list      */
	struct Name_List *nl_last;	/* Pointer to the last one	*/
	struct Name_List *nl_point;	/* Pointer to the last one	*/
        struct Group_List *next;        /* A pointer to the next one.   */
    } ;

