
/* **********************************************************************
   * MailList.CPP                                                       *
   *                                                                    *
   * Copyright by Fredric L. Rice, October 1994.                        *
   *                                                                    *
   * The Skeptic Tank, 1:102/890.0, (818) 335-9601.                     *
   *                                                                    *
   ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloc.h>
#include <conio.h>
#include <dir.h>
#include <time.h>
#include <ctype.h>
#include "maillist.hpp"

/* **********************************************************************
   * Window display stuff.                                              *
   *                                                                    *
   ********************************************************************** */

#define Window_Line     5
#define Window_Begin    10
#define Window_End      70
#define Window_Length   (Window_End - Window_Begin)

/* **********************************************************************
   * Error messages stuff.                                              *
   *                                                                    *
   ********************************************************************** */

#define Err_Msg_No_Mail_Directory       0
#define Err_Msg_No_From_Name            1
#define Err_Msg_No_Origin               2
#define Err_Msg_Out_Of_Memory           3
#define Err_Msg_Directory_Before_Group  4
#define Err_Msg_Duplicate_Directory     5
#define Err_Msg_From_Before_Group       6
#define Err_Msg_Duplicate_From_Name     7
#define Err_Msg_From_Name_Too_Long      8
#define Err_Msg_Origin_Before_Group     9
#define Err_Msg_Duplicate_Origin        10
#define Err_Msg_User_Before_Group       11
#define Err_Msg_Name_Bad_Format         12
#define Err_Msg_Name_Too_Large          13
#define Err_Msg_Strange_Address         14
#define Err_Msg_Cant_Find_Configuration 15
#define Err_Msg_Cant_Create_Message     16
#define Err_Msg_Cant_Write_Message      17
#define Err_Msg_Cant_File_Attach        18
#define Err_Msg_Text_File_Missing       19
#define Err_Msg_Total                   20

/* **********************************************************************
   * Create an array of pointers to our error message.                  *
   *                                                                    *
   ********************************************************************** */

    static char *Error_Messages[Err_Msg_Total + 1];

/* **********************************************************************
   * We maintain a linked list of linked lists for those people whom we *
   * wish to make a mailing to.                                         *
   *                                                                    *
   ********************************************************************** */

    static struct Group_List *gl_first, *gl_last, *gl_point;

/* **********************************************************************
   * The message file format offered here is Fido format which has      *
   * been tested with OPUS and Dutchie. It represents the latest        *
   * format that I know about.  The format, in fact, appears to be      *
   * locked-up in the past, prompting the need for kludge-lines to      *
   * handle route information not included in the mindset of the early  *
   * pioneers of FidoNet.                                               *
   *                                                                    *
   ********************************************************************** */

#define MSG_From_Length         36
#define MSG_To_Length           36
#define MSG_Subject_Length      72
#define MSG_Date_Length         20

    struct fido_msg {
        char from[MSG_From_Length];       /* Who the message is from      */
        char to[MSG_To_Length];           /* Who the message to to        */
        char subject[MSG_Subject_Length]; /* The subject of the message   */
        char date[MSG_Date_Length];       /* Message creation date/time   */
        USHORT times;              /* Number of time the message was read */
        USHORT destination_node;   /* Intended destination node           */
        USHORT originate_node;     /* The originator node of the message  */
        USHORT cost;               /* Cost to send this message           */
        USHORT originate_net;      /* The originator net of the message   */
        USHORT destination_net;    /* Intended destination net number     */
        USHORT destination_zone;   /* Intended zone for the message       */
        USHORT originate_zone;     /* The zone of the originating system  */
        USHORT destination_point;  /* Is there a point to destination?    */
        USHORT originate_point;    /* Point that originated the message   */
        USHORT reply;              /* Thread to previous reply            */
        USHORT attribute;          /* Message type                        */
        USHORT upwards_reply;      /* Thread to next message reply        */
    } message;                     /* Create one of this structure.       */

/* **********************************************************************
   * 'Attribute' bit definitions, some of which we will use             *
   *                                                                    *
   ********************************************************************** */

#define Fido_Private            0x0001
#define Fido_Crash              0x0002
#define Fido_Read               0x0004
#define Fido_Sent               0x0008
#define Fido_File_Attach        0x0010
#define Fido_Forward            0x0020
#define Fido_Orphan             0x0040
#define Fido_Kill               0x0080
#define Fido_Local              0x0100
#define Fido_Hold               0x0200
#define Fido_Reserved1          0x0400
#define Fido_File_Request       0x0800
#define Fido_Ret_Rec_Req        0x1000
#define Fido_Ret_Rec            0x2000
#define Fido_Req_Audit_Trail    0x4000
#define Fido_Update_Req         0x8000

/* **********************************************************************
   * Here are the errorlevels we are allowed to exit with.              *
   *                                                                    *
   ********************************************************************** */

#define No_Problem              0
#define Cant_Find_Config_File   10
#define No_Memory               11
#define Configuration_Bad       12
#define Cant_Create_Message     13
#define Cant_Write_Message      14
#define Cant_Find_Errors_File   15
#define Error_File_Corrupted    16
#define Cant_File_Attach        17
#define Text_File_Missing       18

/* **********************************************************************
   * Define local storage.                                              *
   *                                                                    *
   ********************************************************************** */

    static USHORT group_count;
    static USHORT name_count;
    static USHORT sysop_zone;
    static USHORT sysop_network;
    static USHORT sysop_node;
    static USHORT sysop_point;
    static USHORT uucp_zone;
    static USHORT uucp_network;
    static USHORT uucp_node;
    static USHORT uucp_point;
    static BOOL want_immediate;
    static BOOL want_crash;
    static BOOL want_private;
    static BOOL want_kill_after_sent;
    static BOOL want_diag;
    static BOOL want_file_request;
    static BOOL want_file_attach;
    static BOOL file_requested;
    static BOOL file_attached;
    static BOOL nodelist_opened;
    static char configuration_directory[201];
    static char error_message_directory[201];
    static char nodelist_file[201];
    static char message_subject[MSG_Subject_Length + 1];
    static char message_file[101];
    static USHORT test_zone;
    static USHORT test_network;
    static USHORT search_count;
    static FILE *nodelist;
    static FILE *fin;

    static char *num_to_month[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    } ;

    static char *num_to_day[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    } ;

/* **********************************************************************
   * See if there is an interrupt on the keyboard.                      *
   *                                                                    *
   ********************************************************************** */

static USHORT test_keyboard_interrupt(void)
{
    if (kbhit() == 0)
        return(FALSE);

    if (getch() == 27)
        return(TRUE);

    return(FALSE);
}

/* **********************************************************************
   * Create a window and put stuff into it.                             *
   *                                                                    *
   ********************************************************************** */

static void text_window(char which, char *additional)
{
    char up_to_here[101], *last_space;
    char count, space_count;
    USHORT line, collumn;
    char *message;

/*
 * Get a pointer to our error message array
 */

    if (which >= Err_Msg_Total)
        return;

    message = Error_Messages[which];

    if (message == (char *)NULL)
        return;

/*
 * Start here
 */

    line = Window_Line;
    collumn = Window_Begin;

/*
 * DIsplay the message in a window
 */

    textcolor(LIGHTRED);

    gotoxy(collumn, line++);
    (void)cprintf("ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
    (void)cprintf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿");

    gotoxy(collumn, line++);
    (void)cprintf("³                               ");
    (void)cprintf("                              ³");

    while (*message) {
        for (count = 0; count < 100; count++) {
            up_to_here[count] = ' ';
        }

        count = 0;
        space_count = 0;
        skipspace(message);
        last_space = message;

        while (*message && count < Window_Length) {
            if (*message == ' ') {
                last_space = message;
                space_count = count;
            }

            up_to_here[count++] = *message++;
        }

        if (*message) {
            message = last_space;

            for (; space_count < Window_Length; space_count++) {
                up_to_here[space_count] = ' ';
            }
        }

        up_to_here[Window_Length - 1] = (char)NULL;
        gotoxy(collumn, line++);
        (void)cprintf("%c %s %c", '³', up_to_here, '³');
    }

    gotoxy(collumn, line++);
    (void)cprintf("³                               ");
    (void)cprintf("                              ³");

    if (additional == (char *)NULL) {
        gotoxy(collumn, line++);
        (void)cprintf("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
        (void)cprintf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
    }
    else {
        (void)strncpy(up_to_here, additional, Window_Length);
        up_to_here[Window_Length - 1] = (char)NULL;

        for (count = strlen(up_to_here); count < Window_Length - 1; count++)
            (void)strcat(up_to_here, " ");

        gotoxy(collumn, line++);
        (void)cprintf("ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
        (void)cprintf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´");

        gotoxy(collumn, line++);
        (void)cprintf("³                               ");
        (void)cprintf("                              ³");

        gotoxy(collumn, line++);
        (void)cprintf("%c %s %c", '³', up_to_here, '³');

        gotoxy(collumn, line++);
        (void)cprintf("³                               ");
        (void)cprintf("                              ³");

        gotoxy(collumn, line++);
        (void)cprintf("ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
        (void)cprintf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ");
    }
}

/* **********************************************************************
   * Plug all kludge lines.                                             *
   *                                                                    *
   ********************************************************************** */

static void include_kludge(USHORT highest_move, FILE *fout)
{
    char record[201];
    time_t the_time;

/*
 * The MSGID contains a unique number so that duplications can be
 * searched for.  We use the highest message number in the move
 * directory and the current date and time.  That should be fine.
 */

    the_time = time(NULL);

    (void)sprintf(record, "%cMSGID: %d:%d/%d.%d %08lx%c%c",
        0x01,
        message.originate_zone,
        message.originate_net,
        message.originate_node,
        message.originate_point,
        (unsigned long)the_time * (highest_move + 1),
        0x0d, 0x0a);

    (void)fputs(record, fout);

/*
 * See if it should be kludged as immediate and direct.
 */

    if (want_immediate) {
        (void)sprintf(record, "%cFLAGS IMM, DIR%c%c", 0x01, 0x0d, 0x0a);
        (void)fputs(record, fout);
    }       

/*
 * Add the 'topt' and 'fmpt' kludges if needed
 */

    if (sysop_point != 0) {

        (void)sprintf(record, "%cFMPT %d%c%c",
            0x01, message.originate_point, 0x0d, 0x0a);

        (void)fputs(record, fout);
    }

    if (message.destination_point != 0) {

        (void)sprintf(record, "%cTOPT %d%c%c",
	    0x01, message.destination_point, 0x0d, 0x0a);

        (void)fputs(record, fout);
    }

/*
 * Always add the 'INTL' kludge
 */

    (void)sprintf(record, "%cINTL %d:%d/%d.%d %d:%d/%d.%d%c%c",
        0x01,
        message.destination_zone,
        message.destination_net,
        message.destination_node,
        message.destination_point,
	sysop_zone,
	sysop_network,
	sysop_node,
	sysop_point,
        0x0d, 0x0a);

    (void)fputs(record, fout);

/*
 * Give us a product ID
 */

    (void)sprintf(record, "%cPID: MailList %s 1:102/890.0%c%c",
        0x01, The_Version, 0x0d, 0x0a);

    (void)fputs(record, fout);
}

/* **********************************************************************
   * Find the highest message number and return it.                     *
   *                                                                    *
   ********************************************************************** */

static short find_highest_message_number(char *directory)
{
    char result;
    short highest_message_number = 0;
    char directory_search[100];
    struct ffblk file_block;

/*
 * Build the directory name to search for, include \ if needed
 */

    (void)strcpy(directory_search, directory);

    if (directory[strlen(directory) - 1] != '\\')
        (void)strcat(directory, "\\");

    (void)strcat(directory_search, "*.MSG");

/*
 * See if we have at least one
 */

    result = findfirst(directory_search, &file_block, 0x16);

    if (! result) {
        if (atoi(file_block.ff_name) > highest_message_number) {
            highest_message_number = atoi(file_block.ff_name);
        }
    }

/*
 * Scan all messages until we know the highest message number
 */

    while (! result) {
        result = findnext(&file_block);

        if (! result) {
            if (atoi(file_block.ff_name) > highest_message_number) {
                highest_message_number = atoi(file_block.ff_name);
            }
        }
    }

/*
 * Return the value
 */

    return(highest_message_number);
}

/* **********************************************************************
   * Check to see if the last group was complete.                       *
   *                                                                    *
   ********************************************************************** */

static void validate_last_group(void)
{
    if (gl_last != (struct Group_List *)NULL) {
        if (gl_last->mail_directory == (char *)NULL) {
            text_window(Err_Msg_No_Mail_Directory, gl_last->group_name);
            fcloseall();
            exit(Configuration_Bad);
        }

        if (gl_last->from_name == (char *)NULL) {
            text_window(Err_Msg_No_From_Name, gl_last->group_name);
            fcloseall();
            exit(Configuration_Bad);
        }

        if (gl_last->origin == (char *)NULL) {
            text_window(Err_Msg_No_Origin, gl_last->group_name);
            fcloseall();
            exit(Configuration_Bad);
        }
    }
}

/* **********************************************************************
   * Plug the group name.                                               *
   *                                                                    *
   ********************************************************************** */

static void plug_group(char *group_name)
{
    validate_last_group();

    gl_point = (struct Group_List *)farmalloc(sizeof(struct Group_List));

    if (gl_point == (struct Group_List *)NULL) {
        text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
        fcloseall();
        exit(No_Memory);
    }

/*
 * Take care of Name List linked list
 */

    gl_point->nl_first = (struct Name_List *)NULL;
    gl_point->nl_last = (struct Name_List *)NULL;
    gl_point->nl_point = (struct Name_List *)NULL;

/*
 * Initialize name count
 */

    gl_point->name_count = 0;

/*
 * No mailing directory defined as yet
 */

    gl_point->mail_directory = (char *)NULL;

/*
 * No from name either
 */

    gl_point->from_name = (char *)NULL;

/*
 * No origin either
 */

    gl_point->origin = (char *)NULL;

/*
 * Take care of Group Name
 */

    gl_point->group_name = (char *)farmalloc(strlen(group_name) + 1);

    if (gl_point->group_name == (char *)NULL) {
        text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
        fcloseall();
        exit(No_Memory);
    }

    (void)strcpy(gl_point->group_name, group_name);

/*
 * Append the entry in the linked list.
 */

    gl_point->next = (struct Group_List *)NULL;

    if (gl_first == (struct Group_List *)NULL) {
        gl_first = gl_point;
    }
    else {
        gl_last->next = gl_point;
    }

    gl_last = gl_point;

    group_count++;
}

/* **********************************************************************
   * Plug the outbound mail directory.                                  *
   *                                                                    *
   ********************************************************************** */

static void plug_directory(char *atpoint)
{
    if (gl_last == (struct Group_List *)NULL) {
        text_window(Err_Msg_Directory_Before_Group, (char *)NULL);
        fcloseall();
        exit(Configuration_Bad);
    }

    if (gl_last->mail_directory != (char *)NULL) {
        text_window(Err_Msg_Duplicate_Directory, gl_last->group_name);
        fcloseall();
        exit(Configuration_Bad);
    }

    gl_last->mail_directory = (char *)farmalloc(strlen(atpoint) + 1);

    if (gl_last->mail_directory == (char *)NULL) {
        text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
        fcloseall();
        exit(No_Memory);
    }

    (void)strcpy(gl_last->mail_directory, atpoint);
}

/* **********************************************************************
   * Store the from name.                                               *
   *                                                                    *
   ********************************************************************** */

static void plug_from_name(char *atpoint)
{
    if (gl_last == (struct Group_List *)NULL) {
        text_window(Err_Msg_From_Before_Group, (char *)NULL);
        fcloseall();
        exit(Configuration_Bad);
    }

    if (gl_last->from_name != (char *)NULL) {
        text_window(Err_Msg_Duplicate_From_Name, gl_last->group_name);
        fcloseall();
        exit(Configuration_Bad);
    }

    if (strlen(atpoint) >= MSG_From_Length - 1) {
        text_window(Err_Msg_From_Name_Too_Long, atpoint);
        fcloseall();
        exit(Configuration_Bad);
    }

    gl_last->from_name = (char *)farmalloc(strlen(atpoint) + 1);

    if (gl_last->from_name == (char *)NULL) {
        text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
        fcloseall();
        exit(No_Memory);
    }

    (void)strcpy(gl_last->from_name, atpoint);
}

/* **********************************************************************
   * Store the origins string.                                          *
   *                                                                    *
   ********************************************************************** */

static void plug_origin(char *atpoint)
{
    if (gl_last == (struct Group_List *)NULL) {
        text_window(Err_Msg_Origin_Before_Group, (char *)NULL);
        fcloseall();
        exit(Configuration_Bad);
    }

    if (gl_last->origin != (char *)NULL) {
        text_window(Err_Msg_Duplicate_Origin, gl_last->group_name);
        fcloseall();
        exit(Configuration_Bad);
    }

    gl_last->origin = (char *)farmalloc(strlen(atpoint) + 1);

    if (gl_last->origin == (char *)NULL) {
        text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
        fcloseall();
        exit(No_Memory);
    }

    (void)strcpy(gl_last->origin, atpoint);
}

/* **********************************************************************
   * Try to validate the address.                                       *
   *                                                                    *
   ********************************************************************** */

static USHORT validate_address(char *address, char *name)
{
    if (! strnicmp(name, "network", 7)) {
        if (nodelist_file[0] != (char)NULL) {
            if (strchr(address, ':') != (char *)NULL) {
                return(TRUE);
            }
        }
    }

    if (strchr(address, '@') != (char *)NULL)
        return(TRUE);

    if (strchr(address, ':') == (char *)NULL)
        return(FALSE);

    if (strchr(address, '/') == (char *)NULL)
        return(FALSE);

    if (strchr(address, '.') == (char *)NULL)
        return(FALSE);

    return(TRUE);
}

/* **********************************************************************
   * If the nodelist entry is a new host, return FALSE as it's the end  *
   * of the network.  Otherwise extract the name and address of the     *
   * nodelist entry and plug them into the buffer pointers then return  *
   * TRUE.                                                              *
   *                                                                    *
   ********************************************************************** */

static USHORT derive_address(char *name,
    char *address,
    char *atpoint,
    USHORT get_node)
{
    USHORT test_node;
    char hold_name[101];
    USHORT count;

    if (! strnicmp(atpoint, "host", 4)) {
        return(FALSE);
    }

    if (! strnicmp(atpoint, "region", 6)) {
        return(FALSE);
    }

    if (! strnicmp(atpoint, "zone", 4)) {
        return(FALSE);
    }

/*
 * Get to the first ,
 */

    while (*atpoint && *atpoint != ',')
        atpoint++;

    if (! *atpoint)
        return(FALSE);

/*
 * Past it
 */

    atpoint++;

/*
 * Get the node
 */

    test_node = atoi(atpoint);

    if (! get_node)
        test_node = 0;

/*
 * Skip past the node
 */

    while (*atpoint && *atpoint != ',')
        atpoint++;

    if (! *atpoint)
        return(FALSE);

    atpoint++;

/*
 * Skip past system name
 */

    while (*atpoint && *atpoint != ',')
        atpoint++;

    if (! *atpoint)
        return(FALSE);

    atpoint++;

/*
 * Skip past location
 */

    while (*atpoint && *atpoint != ',')
        atpoint++;

    if (! *atpoint)
        return(FALSE);

    atpoint++;

/*
 * Plug users name
 */

    count = 0;

    while (*atpoint && *atpoint != ',') {
        if (*atpoint != '_') {
            hold_name[count++] = *atpoint;
        }
        else {
            hold_name[count++] = ' ';
        }

        hold_name[count] = (char)NULL;
        atpoint++;
    }

    (void)sprintf(address, "%d:%d/%d.0", test_zone, test_network, test_node);
    (void)strcpy(name, hold_name);
    return(TRUE);
}

/* **********************************************************************
   * Get the nodelist entry for the address.                            *
   *                                                                    *
   ********************************************************************** */

static USHORT get_nodelist_entry(char *name, char *address)
{
    char record[201], *atpoint;

/*
 * Double check
 */

    if (nodelist_file[0] == (char)NULL)
        return(FALSE);

    textcolor(YELLOW);

/*
 * If we've not opened the file yet, do so and extract the
 * zone and network that we're looking through.
 */

    if (! nodelist_opened) {
        if ((nodelist = fopen(nodelist_file, "rt")) == (FILE *)NULL)
            return(FALSE);

        nodelist_opened = TRUE;

        test_zone = atoi(address);
        atpoint = strchr(address, ':');

        if (atpoint == (char *)NULL)
            return(FALSE);

        atpoint++;
        skipspace(atpoint);          
        test_network = atoi(atpoint);

        gotoxy(10, 10);
        clreol();
        textcolor(LIGHTBLUE);

        (void)cprintf("Search for nodes in Zone %d, Network %d",
            test_zone, test_network);

        while (! feof(nodelist)) {
            if (test_keyboard_interrupt()) {
                (void)fclose(nodelist);
                nodelist_opened = FALSE;
                return(FALSE);
            }

            (void)fgets(record, 200, nodelist);

            if (! feof(nodelist)) {
                atpoint = record;
                skipspace(atpoint);

                gotoxy(50, 23);
                (void)printf("ZSearch: %5d", ++search_count);

                if (! strnicmp(atpoint, "zone", 4)) {
                    atpoint += 5;
                    skipspace(atpoint);

                    if (atoi(atpoint) == test_zone) {

                        while (! feof(nodelist)) {
                            if (test_keyboard_interrupt()) {
                                (void)fclose(nodelist);
                                nodelist_opened = FALSE;
                                return(FALSE);
                            }

                            (void)fgets(record, 200, nodelist);

                            if (! feof(nodelist)) {
                                atpoint = record;
                                skipspace(atpoint);

                                gotoxy(50, 23);
                                (void)printf("HSearch: %5d", ++search_count);

                                if (! strnicmp(atpoint, "host", 4)) {
                                    atpoint += 5;
                                    skipspace(atpoint);

                                    if (atoi(atpoint) == test_network) {
                                        atpoint--;

                                        if (! derive_address(name, address,
                                                atpoint, FALSE)) {

                                            (void)fclose(nodelist);
                                            nodelist_opened = FALSE;
                                            return(FALSE);
                                        }

                                        return(TRUE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        (void)fclose(nodelist);
        nodelist_opened = FALSE;
        return(FALSE);
    }

    if (test_keyboard_interrupt()) {
        (void)fclose(nodelist);
        nodelist_opened = FALSE;
        return(FALSE);
    }

/*
 * Get the next one
 */

    (void)fgets(record, 200, nodelist);

    if (feof(nodelist)) {
        (void)fclose(nodelist);
        nodelist_opened = FALSE;
        return(FALSE);
    }

    gotoxy(50, 23);
    (void)printf("NSearch: %5d", ++search_count);

    atpoint = record;
    skipspace(atpoint);

    if (! derive_address(name, address, atpoint, TRUE)) {
        (void)fclose(nodelist);
        nodelist_opened = FALSE;
        return(FALSE);
    }

    return(TRUE);
}

/* **********************************************************************
   * Plug the users name.                                               *
   *                                                                    *
   ********************************************************************** */

static void plug_name(char *atpoint)
{
    char user_name[101];
    char user_address[101];
    USHORT count;
    USHORT bail_out;
    BOOL nodelist_searching;
    BOOL allow_create;
    char *original;

    if (gl_last == (struct Group_List *)NULL) {
        text_window(Err_Msg_User_Before_Group, (char *)NULL);
        fcloseall();
        exit(Configuration_Bad);
    }

    original = atpoint;

/*
 * Extract the users name
 */

    count = 0;

    while (*atpoint && *atpoint != ',')
        user_name[count++] = *atpoint++;

    user_name[count] = (char)NULL;

    if (! *atpoint) {
        text_window(Err_Msg_Name_Bad_Format, original);
        fcloseall();
        exit(Configuration_Bad);
    }

/*
 * See if it will fit into the message header
 */

    if (count >= MSG_To_Length - 1) {
        text_window(Err_Msg_Name_Too_Large, original);
        fcloseall();
        exit(Configuration_Bad);
    }

/*
 * Skip past the comma and empty space
 */

    atpoint++;
    skipspace(atpoint);

/*
 * Extract address
 */

    count = 0;

    while (*atpoint)
        user_address[count++] = *atpoint++;

    user_address[count] = (char)NULL;

    if (! validate_address(user_address, user_name)) {
        text_window(Err_Msg_Strange_Address, original);
        fcloseall();
        exit(Configuration_Bad);
    }

/*
 * Allocate space for the users name(s) structure(s)
 */

    bail_out = 1;
    nodelist_searching = FALSE;

    if (! strnicmp(user_name, "network", 7)) {
        if (nodelist_file[0] != (char)NULL) {
            bail_out = 9999;
            nodelist_searching = TRUE;
        }
    }

    while (bail_out > 0) {
        bail_out--;
        allow_create = TRUE;

        if (nodelist_searching) {
            if (! get_nodelist_entry(user_name, user_address)) {
                bail_out = 0;
                allow_create = FALSE;
            }
        }

        if (allow_create) {
            gl_last->nl_point = (struct Name_List *)
                farmalloc(sizeof(struct Name_List));

            if (gl_last->nl_point == (struct Name_List *)NULL) {
                text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
                fcloseall();
                exit(No_Memory);
            }

/*
 * Allocate memory for the users name
 */

            gl_last->nl_point->name = (char *)
                farmalloc(strlen(user_name) + 1);

            if (gl_last->nl_point->name == (char *)NULL) {
                text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
                fcloseall();
                exit(No_Memory);
            }

            (void)strcpy(gl_last->nl_point->name, user_name);

/*
 * Allocate memory for the users address
 */

            gl_last->nl_point->address = (char *)
                farmalloc(strlen(user_address) + 1);

            if (gl_last->nl_point->address == (char *)NULL) {
                text_window(Err_Msg_Out_Of_Memory, (char *)NULL);
                fcloseall();
                exit(No_Memory);
            }

            (void)strcpy(gl_last->nl_point->address, user_address);

/*
 * Incriment name count
 */

            gl_last->name_count++;

/*
 * Append the entry in the linked list.
 */

            gl_last->nl_point->next = (struct Name_List *)NULL;

            if (gl_last->nl_first == (struct Name_List *)NULL) {
                gl_last->nl_first = gl_last->nl_point;
            }
            else {
                gl_last->nl_last->next = gl_last->nl_point;
            }

            gl_last->nl_last = gl_last->nl_point;

            name_count++;
        }
    }
}

/* **********************************************************************
   * Extract the configuration file information.  Do it now!            *
   *                                                                    *
   ********************************************************************** */
   
static void extract_configuration(void)
{
    FILE *fin;
    char record[201], *atpoint;

    if ((fin = fopen(configuration_directory, "rt")) == (FILE *)NULL) {
        text_window(Err_Msg_Cant_Find_Configuration, configuration_directory);
        fcloseall();
        exit(Cant_Find_Config_File);
    }

    while (! feof(fin)) {
        (void)fgets(record, 200, fin);

        if (! feof(fin)) {
            atpoint = record;
            skipspace(atpoint);
            atpoint[strlen(atpoint) - 1] = (char)NULL;

            if (! strnicmp(atpoint, "group", 5)) {
                atpoint += 5;
                skipspace(atpoint);
                plug_group(atpoint);
            }
	    else if (! strnicmp(atpoint, "directory", 9)) {
                atpoint += 9;
                skipspace(atpoint);
                plug_directory(atpoint);
            }
            else if (! strnicmp(atpoint, "from", 4)) {
                atpoint += 4;
                skipspace(atpoint);
                plug_from_name(atpoint);
            }
            else if (! strnicmp(atpoint, "origin", 6)) {
                atpoint += 6;
                skipspace(atpoint);
                plug_origin(atpoint);
            }
            else if (! strnicmp(atpoint, "name", 4)) {
                atpoint += 4;
                skipspace(atpoint);
                plug_name(atpoint);
            }
            else if (! strnicmp(atpoint, "nodelist", 8)) {
                atpoint += 8;
                skipspace(atpoint);
                (void)strcpy(nodelist_file, atpoint);
            }
            else if (! strnicmp(atpoint, "zone", 4)) {
                atpoint += 4;
                skipspace(atpoint);
                sysop_zone = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "network", 7)) {
                atpoint += 7;
                skipspace(atpoint);
		sysop_network = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "node", 4)) {
                atpoint += 4;
                skipspace(atpoint);
                sysop_node = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "point", 5)) {
                atpoint += 5;
                skipspace(atpoint);
                sysop_point = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "uucpzone", 8)) {
                atpoint += 8;
                skipspace(atpoint);
                uucp_zone = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "uucpnetwork", 11)) {
                atpoint += 11;
                skipspace(atpoint);
                uucp_network = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "uucpnode", 8)) {
                atpoint += 8;
                skipspace(atpoint);
                uucp_node = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "uucppoint", 9)) {
                atpoint += 9;
                skipspace(atpoint);
                uucp_point = atoi(atpoint);
            }
            else if (! strnicmp(atpoint, "immediate", 9)) {
                atpoint += 9;
                skipspace(atpoint);

		if (! strnicmp(atpoint, "yes", 3)) {
                    want_immediate = TRUE;
                }
            }
            else if (! strnicmp(atpoint, "crash", 5)) {
                atpoint += 5;
                skipspace(atpoint);

		if (! strnicmp(atpoint, "yes", 3)) {
                    want_crash = TRUE;
                }
            }
            else if (! strnicmp(atpoint, "private", 7)) {
                atpoint += 7;
                skipspace(atpoint);

		if (! strnicmp(atpoint, "yes", 3)) {
                    want_private = TRUE;
                }
            }
            else if (! strnicmp(atpoint, "killsent", 8)) {
                atpoint += 8;
                skipspace(atpoint);

		if (! strnicmp(atpoint, "yes", 3)) {
                    want_kill_after_sent = TRUE;
                }
            }
            else if (! strnicmp(atpoint, "filerequest", 11)) {
                atpoint += 11;
                skipspace(atpoint);

		if (! strnicmp(atpoint, "yes", 3)) {
                    want_file_request = TRUE;
                }
            }
            else if (! strnicmp(atpoint, "fileattach", 10)) {
                atpoint += 10;
                skipspace(atpoint);

		if (! strnicmp(atpoint, "yes", 3)) {
                    want_file_attach = TRUE;
                }
            }
        }
    }

    (void)fclose(fin);

    validate_last_group();
}

/* **********************************************************************
   * Initialize this.                                                   *
   *                                                                    *
   ********************************************************************** */
   
static void initialize(void)
{
    USHORT loop;
    char *env;

/*
 * Get our environment variable to determine the path
 * to our configuration file and error message file
 */

    if (NULL == (env = getenv("MAILLIST"))) {
        (void)strcpy(configuration_directory, "MAILLIST.CFG");
        (void)strcpy(error_message_directory, "MAILLIST.ERR");
    }
    else {
        (void)strcpy(configuration_directory, env);
        (void)strcpy(error_message_directory, env);

        if (configuration_directory[strlen(configuration_directory) - 1] != '\\') {
            (void)strcat(configuration_directory, "\\");
            (void)strcat(error_message_directory, "\\");
        }

        (void)strcat(configuration_directory, "MAILLIST.CFG");
        (void)strcat(error_message_directory, "MAILLIST.ERR");
    }                                                      

/*
 * Zero out our local memory
 */

    group_count = 0;
    name_count = 0;
    sysop_zone = 0;
    sysop_network = 0;
    sysop_node = 0;
    sysop_point = 0;
    uucp_zone = 0;
    uucp_network = 0;
    uucp_node = 0;
    uucp_point = 0;
    want_immediate = FALSE;
    want_crash = FALSE;
    want_private = TRUE;
    want_kill_after_sent = TRUE;
    want_file_request = FALSE;
    want_file_attach = FALSE;
    file_requested = FALSE;
    file_attached = FALSE;

    gl_first = gl_last = gl_point = (struct Group_List *)NULL;

    for (loop = 0; loop < Err_Msg_Total; loop++) {
        Error_Messages[loop] = (char *)NULL;
    }

    nodelist_opened = FALSE;
    test_zone = 0;
    test_network = 0;
    search_count = 0;
}

/* **********************************************************************
   * Display the group information.                                     *
   *                                                                    *
   ********************************************************************** */

static void display_groups(void)
{
    USHORT line, collumn, count;

    line = 4;
    collumn = 1;
    count = 1;

    gl_point = gl_first;

    while (gl_point) {
	gotoxy(collumn, line++);

        textcolor(LIGHTBLUE);

        (void)cprintf("%2d) %03d %s",
            count++,
            gl_point->name_count,
            gl_point->group_name);

        if (line == 20) {
            line = 4;
            collumn = 40;
        }

        gl_point = gl_point->next;
    }
}

/* **********************************************************************
   * Say hello.                                                         *
   *                                                                    *
   ********************************************************************** */

static void say_hello(void)
{
    clrscr();

    gotoxy(1, 1);

    textcolor(LIGHTGREEN);

    (void)cprintf(
        "MailList Ver %s (%s)  FidoNet: %d:%d/%d.%d UUCP %d:%d/%d.%d",
        The_Version, __DATE__,
        sysop_zone, sysop_network, sysop_node, sysop_point,
        uucp_zone, uucp_network, uucp_node, uucp_point);

    gotoxy(1, 2);

    textcolor(LIGHTGREEN);

    (void)cprintf("There are %d names defined in %d groups",
        name_count, group_count);
}

/* **********************************************************************
   * Get the workin group into gl_point.                                *
   *                                                                    *
   ********************************************************************** */

static USHORT get_working_group(void)
{
    char input[51];
    USHORT option;

    gotoxy(1, 23);
    textcolor(YELLOW);
    (void)cprintf("Enter group number to mail to: ");
    (void)fgets(input, 50, stdin);
    option = atoi(input);

    if (option < 1 || option > group_count)
	return(FALSE);

    gl_point = gl_first;

    while (--option > 0)
        gl_point = gl_point->next;

    if (gl_point != (struct Group_List *)NULL)
        return(TRUE);

    return(FALSE);
}

/* **********************************************************************
   * Display the names of the group.                                    *
   *                                                                    *
   ********************************************************************** */

static USHORT display_names(void)
{
    USHORT line, collumn;

/*
 * Make sure there are some names here
 */

    if (gl_point->name_count == 0)
        return(FALSE);

    clrscr();
    line = 4;
    collumn = 1;

/*
 * Display the header
 */

    gotoxy(1, 1);

    textcolor(LIGHTBLUE);

    (void)cprintf("Group: %s, directory %s %d names",
        gl_point->group_name,
        gl_point->mail_directory,
        gl_point->name_count);

    gotoxy(1, 2);

    textcolor(LIGHTBLUE);

    (void)cprintf("Messages will be from: %s", gl_point->from_name);

/*
 * Go through the list and display each name
 */

    gl_point->nl_point = gl_point->nl_first;

    while (gl_point->nl_point) {
        gotoxy(collumn, line++);

        textcolor(LIGHTGREEN);

        (void)cprintf(" %s  %s",
            gl_point->nl_point->name,
            gl_point->nl_point->address);

        if (line >= 20) {
            if (collumn == 40) {
                gotoxy(1, 22);
                textcolor(LIGHTRED);
                (void)cprintf("Not all names in the group are displayed.");
                return(TRUE);
            }

            line = 4;
            collumn = 40;
        }

        gl_point->nl_point = gl_point->nl_point->next;
    }

    return(TRUE);
}

/* **********************************************************************
   * Get the message file.                                              *
   *                                                                    *
   ********************************************************************** */

static USHORT get_message_file(void)
{
    char input[101];

    gotoxy(1, 23);
    textcolor(YELLOW);
    (void)cprintf("Enter message file path and name to send to all: ");
    (void)fgets(input, 100, stdin);
    input[strlen(input) - 1] = (char)NULL;
    (void)strcpy(message_file, input);

    gotoxy(1, 23);
    clreol();
    textcolor(LIGHTGREEN);
    (void)cprintf("Opening file: [%s]", input);

    if ((fin = fopen(input, "rt")) == (FILE *)NULL)
        return(FALSE);

    return(TRUE);
}

/* **********************************************************************
   * See if it's for FidoNet or Internet.  Address accordingly.         *
   *                                                                    *
   ********************************************************************** */

static USHORT extract_addressing_information(void)
{
    char *atpoint;

    if (strchr(gl_point->nl_point->address, '@') != (char *)NULL) {
        (void)strcpy(message.to, "uucp");

        message.destination_zone = uucp_zone;
        message.destination_net = uucp_network;
        message.destination_node = uucp_node;
        message.destination_point = uucp_point;
        return(TRUE);
    }
    else {
        (void)strcpy(message.to, gl_point->nl_point->name);

        message.destination_zone = 0;
        message.destination_net = 0;
        message.destination_node = 0;
        message.destination_point = 0;

        atpoint = gl_point->nl_point->address;

        message.destination_zone = atoi(atpoint);

        while (*atpoint && *atpoint != ':')
            atpoint++;

        if (*atpoint) {
            atpoint++;

            message.destination_net = atoi(atpoint);

	    while (*atpoint && *atpoint != '/')
		atpoint++;

	    if (*atpoint) {
		atpoint++;

		message.destination_node = atoi(atpoint);

		while (*atpoint && *atpoint != '.')
		    atpoint++;

		if (*atpoint) {
		    message.destination_point = atoi(atpoint);
		}
            }
        }

        return(FALSE);
    }
}

/* **********************************************************************
   * Send the message to the individual.                                *
   *                                                                    *
   ********************************************************************** */

static void send_message(USHORT message_number)
{
    char file_name[101];
    FILE *fout;
    USHORT is_uucp;
    USHORT result;
    char record[201];

    (void)sprintf(file_name, "%s%d.MSG",
        gl_point->mail_directory, message_number);

    gotoxy(1, 23);
    clreol();
    textcolor(LIGHTBLUE);
    (void)cprintf("Creating %s", file_name);

    if ((fout = fopen(file_name, "wb")) == (FILE *)NULL) {
        text_window(Err_Msg_Cant_Create_Message, file_name);
        fcloseall();
        exit(Cant_Create_Message);
    }

    is_uucp = extract_addressing_information();

/*
 * If we are a file request and not UUCP, flag it as
 * a file request.
 */

    if (want_file_request) {
        if (file_requested) {
            if (! is_uucp) {
                message.attribute += Fido_File_Request;
            }
        }
    }

/*
 * If we are a file attach and not UUCP, flag it as
 * a file attach.
 */

    if (want_file_attach) {
        if (file_attached) {
            if (! is_uucp) {
                message.attribute += Fido_File_Attach;
            }
        }
    }

/*
 * Write header
 */

    result = fwrite(&message, sizeof(struct fido_msg), 1, fout);

    if (result != 1) {
        text_window(Err_Msg_Cant_Write_Message, file_name);
        fcloseall();
        exit(Cant_Write_Message);
    }

/*
 * If it's to UUCP we write the address first.
 * If FidoNet, we include kludges.
 */

    if (is_uucp) {
        (void)fputs("To: ", fout);
        (void)fputs(gl_point->nl_point->address, fout);
        (void)fputc(0x0d, fout);
        (void)fputc(0x0a, fout);
        (void)fputc(0x0d, fout);
        (void)fputc(0x0a, fout);
        (void)fputs("Attn: ", fout);
        (void)fputs(gl_point->nl_point->name, fout);
        (void)fputc(0x0d, fout);
        (void)fputc(0x0a, fout);
        (void)fputc(0x0d, fout);
        (void)fputc(0x0a, fout);
    }
    else {
	include_kludge(message_number, fout);
    }

/*
 * Message text
 */

    while (! feof(fin)) {
        (void)fgets(record, 200, fin);

        if (! feof(fin)) {
            (void)fputs(record, fout);
            (void)fputc(0x0d, fout);
            (void)fputc(0x0a, fout);
        }
    }

/*
 * Apply origin line
 */

    (void)fputc(0x0d, fout);
    (void)fputc(0x0a, fout);
    (void)fputs("---", fout);
    (void)fputc(0x0d, fout);
    (void)fputc(0x0a, fout);
    (void)fputs(" * Origin: ", fout);
    (void)fputs(gl_point->origin, fout);
    (void)fputc(0x0d, fout);
    (void)fputc(0x0a, fout);
    (void)fclose(fout);
}

/* **********************************************************************
   * day from 1-31, month from 1-12, year from 80                       *
   * Returns 0 for Sunday, etc.                                         *
   *                                                                    *
   *    This function was not written by Fredric Rice. It was taken     *
   *    from the MSGQ150S.LSH archive which is an on-line full          *
   *    screen editor.                                                  *
   *                                                                    *
   ********************************************************************** */

static USHORT zeller(USHORT day, USHORT month, USHORT year)
{
    USHORT age;

    age = (year < 80) ? 20 : 19;

    if ((month -= 2) <= 0) {
        month += 12;
        year--;
    }

    return(((26 * month-2) / 10 +day +year +year / 4 + age / 4 - 2 * age) % 7);
}

/* **********************************************************************
   * Create a new message file for each individual in the list.         *
   *                                                                    *
   ********************************************************************** */

static void send_to_all_names(void)
{
    USHORT highest_message;
    USHORT line, collumn;
    time_t the_time;
    struct tm *current_time;
    USHORT the_month, the_day, the_year, the_hour, the_minute;

/*
 * Get the date and time
 */
    
    (void)time(&the_time);
    current_time = localtime(&the_time);
    the_month = current_time->tm_mon;
    the_day = current_time->tm_mday;
    the_year = current_time->tm_year;
    the_hour = current_time->tm_hour;
    the_minute = current_time->tm_min;

/*
 * Get the highest message number
 */

    highest_message = find_highest_message_number(gl_point->mail_directory);

    gotoxy(1, 2);
    textcolor(LIGHTBLUE);
    (void)cprintf("There are %d messages in the directory", highest_message);

/*
 * Initialize header information
 */

    (void)strcpy(message.from, gl_point->from_name);
    (void)strncpy(message.subject, message_subject, MSG_Subject_Length);
    message.subject[MSG_Subject_Length] = (char)NULL;

    (void)sprintf(message.date,
        "%s %2d %s %2d %02d:%02d",
        num_to_day[zeller(the_day, the_month, the_year)],
        the_day,
        num_to_month[the_month],
        the_year,
        the_hour,
        the_minute);

    message.times = 0;
    message.cost = 0;

    message.originate_zone = sysop_zone;
    message.originate_net = sysop_network;
    message.originate_node = sysop_node;
    message.originate_point = sysop_point;

/*
 * Build message attribute.  Setting it to local has the
 * effect of clearing all bits and starting over with local.
 */

    message.attribute = Fido_Local;

    if (want_kill_after_sent)
        message.attribute += Fido_Kill;

    if (want_crash)
        message.attribute += Fido_Crash;

    if (want_private)
        message.attribute += Fido_Private;

/*
 * Anything else?
 */

    message.reply = 0;
    message.upwards_reply = 0;

/*
 * Redisplay as we create
 */

    line = 4;
    collumn = 1;

/*
 * Go through the list and create a new message for each
 */

    gl_point->nl_point = gl_point->nl_first;

    while (gl_point->nl_point) {
        gotoxy(collumn, line++);

        textcolor(LIGHTGREEN);

        (void)cprintf("*%s  %s",
            gl_point->nl_point->name,
            gl_point->nl_point->address);

        (void)rewind(fin);
        send_message(++highest_message);

        if (line >= 20) {
            line = 4;
            collumn = 40;
        }

        gl_point->nl_point = gl_point->nl_point->next;
    }    

    (void)fclose(fin);
    gotoxy(1, 23);
    clreol();
    textcolor(WHITE);
    (void)cprintf("End of mailing.\n");
}

/* **********************************************************************
   * Get our error messages.                                            *
   *                                                                    *
   ********************************************************************** */

static void extract_error_messages(void)
{
    USHORT error_value;
    FILE *fin;
    char record[201], *atpoint;
    BOOL bail_out;
    USHORT count;

/*
 * Open the file
 */

    if ((fin = fopen(error_message_directory, "rt")) == (FILE *)NULL) {
        textcolor(LIGHTRED);

        (void)cprintf("\n\nI can't find file: %s!\n",
            error_message_directory);

        fcloseall();
        exit(Cant_Find_Errors_File);
    }

/*
 * Read the file and store everything.
 */

    while (! feof(fin)) {
        (void)fgets(record, 200, fin);

        if (! feof(fin)) {
            atpoint = record;
            skipspace(atpoint);
            atpoint[strlen(atpoint) - 1] = (char)NULL;

            if (isdigit(*atpoint)) {
                error_value = atoi(atpoint);

                if (error_value < 0 || error_value >= Err_Msg_Total) {
                    textcolor(LIGHTRED);

                    (void)cprintf("\n\n%s is corrupted!\n",
                        error_message_directory);

                    fcloseall();
                    exit(Error_File_Corrupted);
                }

                if (Error_Messages[error_value] != (char *)NULL) {
                    textcolor(LIGHTRED);

                    (void)cprintf("\n\n%s has a duplicate error number!\n",
                        error_message_directory);

                    fcloseall();
                    exit(Error_File_Corrupted);
                }

                Error_Messages[error_value] = (char *)farmalloc(1024);

                if (Error_Messages[error_value] == (char *)NULL) {
                    textcolor(LIGHTRED);
                    (void)cprintf("\n\nI ran out of memory!\n");
                    fcloseall();
                    exit(No_Memory);
                }                                               

                while (isdigit(*atpoint))
                    atpoint++;

                atpoint++;

                (void)strcpy(Error_Messages[error_value], atpoint);

                bail_out = FALSE;

                while (! feof(fin) && !bail_out) {
                    (void)fgets(record, 200, fin);

                    if (! feof(fin)) {
                        atpoint = record;
                        skipspace(atpoint);
                        atpoint[strlen(atpoint) - 1] = (char)NULL;

                        if (strlen(atpoint) > 2) {
                            (void)strcat(Error_Messages[error_value], " ");
                            (void)strcat(Error_Messages[error_value], atpoint);
                        }
                        else {
                            bail_out = TRUE;
                        }
                    }
                }
            }
        }
    }

/*
 * Close error file
 */

    (void)fclose(fin);

/*
 * Make sure all error messages are defined.
 */

    for (count = 0; count < Err_Msg_Total; count++) {
        if (Error_Messages[count] == (char *)NULL) {
            textcolor(LIGHTRED);

            (void)cprintf("\n\n%s is missing error messages!\n",
                error_message_directory);

            fcloseall();
            exit(Error_File_Corrupted);
        }
    }
}

/* **********************************************************************
   * Set the video back to normal.                                      *
   *                                                                    *
   ********************************************************************** */

static void set_normal_video(void)
{
    normvideo();
}

/* **********************************************************************
   * Main entry point.                                                  *
   *                                                                    *
   ********************************************************************** */
   
void main(USHORT argc, char *argv[])
{
    USHORT loop;
    char input[51];

    nodelist_file[0] = (char)NULL;
    (void)atexit(set_normal_video);
    highvideo();

/*
 * See if we want diagnostics
 */

    for (loop = 1; loop < argc; loop++) {
        if (! strnicmp(argv[loop], "/diag", 5)) {
            want_diag = TRUE;
        }
    }

/*
 * Clear the screen and launch
 */

    if (! want_diag) {
        clrscr();
    }

    initialize();
    extract_error_messages();
    extract_configuration();
    say_hello();
    display_groups();

    if (get_working_group()) {
        if (! display_names()) {
            gotoxy(1, 23);
            clreol();

            textcolor(WHITE);
            (void)cprintf("There are no names in that group!");
        }
        else {
            if (get_message_file()) {
                gotoxy(1, 23);
                clreol();
                textcolor(YELLOW);
                (void)cprintf("Enter subject: ");
                (void)fgets(message_subject, MSG_Subject_Length - 1, stdin);
                message_subject[strlen(message_subject) - 1] = (char)NULL;

                if (want_file_request) {
                    gotoxy(1, 24);
                    clreol();
                    textcolor(LIGHTRED);
                    (void)printf("Should I make this a FileRequest? ");
                    (void)fgets(input, 50, stdin);
                    gotoxy(1, 24);
                    clreol();

                    if (toupper(input[0]) == 'Y') {
                        file_requested = TRUE;
                    }
                }

                if (want_file_attach && !file_requested) {
                    gotoxy(1, 24);
                    clreol();
                    textcolor(LIGHTRED);
                    (void)printf("Should I make this a FileAttach? ");
                    (void)fgets(input, 50, stdin);
                    gotoxy(1, 24);
                    clreol();

                    if (toupper(input[0]) == 'Y') {
                        FILE *test;

                        file_attached = TRUE;

                        if ((test = fopen(message_subject, "rb")) ==
                                (FILE *)NULL) {

                            text_window(Err_Msg_Cant_File_Attach,
                                message_subject);

                            fcloseall();
                            exit(Cant_File_Attach);
                        }

                        (void)fclose(test);
                    }
                }

                send_to_all_names();
            }
            else {
                text_window(Err_Msg_Text_File_Missing, message_file);
                fcloseall();
                exit(Text_File_Missing);
            }
        }
    }

    fcloseall();
    exit(No_Problem);
}


