
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <unqlite.h>

/* Routine to fetch the core temperature */
char *getTemperature(char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char *buffer = (char *) malloc(sizeof (char) * 20);
    fgets(buffer, 20, pipe);
    pclose(pipe);
    return buffer;
}

/* Routine to fetch the timestamp */
char *time_stamp() {

    char *timestamp = (char *) malloc(sizeof (char) * 16);
    time_t ltime;
    ltime = time(NULL);
    struct tm *tm;
    tm = localtime(&ltime);

    sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon,
            tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return timestamp;
}

static const char zBanner[] = {
    "============================================================\n"
    "Volansure		                                     \n"
    "                                       Thing - Main routine \n"
    "============================================================\n"
};

/*
 * Extract the database error log and exit.
 */
static void Fatal(unqlite *pDb, const char *zMsg) {
    if (pDb) {
        const char *zErr;
        int iLen = 0; /* Stupid cc warning */

        /* Extract the database error log */
        unqlite_config(pDb, UNQLITE_CONFIG_ERR_LOG, &zErr, &iLen);
        if (iLen > 0) {
            /* Output the DB error log */
            puts(zErr); /* Always null termniated */
        }
    } else {
        if (zMsg) {
            puts(zMsg);
        }
    }
    /* Manually shutdown the library */
    unqlite_lib_shutdown();
    /* Exit immediately */
    exit(0);
}
/* Forward declaration: Data consumer callback */
static int DataConsumerCallback(const void *pData, unsigned int nDatalen, void *pUserData /* Unused */);

int main(void) { // Main routine
    CURL *curl;

    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);

    char * new_str;
    char * str1 = "thing=T001";
    char * str2 = "sensor=";
    char * str3 = "data=";
    char * str4 = "timestamp=";

    unqlite *pDb; /* Database handle */
    unqlite_kv_cursor *pCur; /* Cursor handle */
    int i, rc;
    char zKey[12];

    puts(zBanner);
    /* Open our database */
    rc = unqlite_open(&pDb, "Volansure.db", UNQLITE_OPEN_CREATE);
    if (rc != UNQLITE_OK) {
        Fatal(0, "Out of memory");
    }
    int count = 0;


    while (1) {
        //         printf("Retrieved line of length %zu :\n", read);
        //         printf("%s", line);

        char *temperature1 = getTemperature("sensors |awk 'NR==8' | awk '{ print $3}'");
        //		printf ("temp1 value: %s",temperature1);
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "http://test.volansure.com");
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            if ((new_str = malloc(strlen(str1) + strlen(str2) + strlen(str3) + strlen(temperature1) + strlen(str4) + 20)) != NULL) {
                new_str[0] = '\0'; // ensures the memory is an empty string
                strcat(new_str, str1);
                strcat(new_str, "&");
                strcat(new_str, str2);
                strcat(new_str, "S1");
                strcat(new_str, "&");
                strcat(new_str, str3);
                strcat(new_str, temperature1);
                strcat(new_str, "&");
                strcat(new_str, str4);
                strcat(new_str, time_stamp());
            } else {
                fprintf(stderr, "malloc failed!\n");
                // exit?
            }
            //	    printf("%s", new_str);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, new_str);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {

                unqlite_util_random_string(pDb, zKey, sizeof (zKey)); // Generate key
   
                /* Store records */
                rc = unqlite_kv_store(pDb, zKey, sizeof (zKey), new_str, strlen(new_str));
                //printf("\n size display1: %s %lu", new_str, strlen(new_str));
                if (rc != UNQLITE_OK) {
                    /* Insertion fail, extract database error log and exit */
                    Fatal(pDb, 0);
                }
                unqlite_commit(pDb);
                count++;
                printf("\ncount: %d", count);
            }
            //	      fprintf(stderr, "curl_easy_perform() failed: %s\n",
            //              curl_easy_strerror(res));
            curl_easy_cleanup(curl);

        }

        char* temperature2 = getTemperature("sensors |awk 'NR==9' | awk '{ print $3}'");
        //		printf ("temp2 value: %s",temperature2);

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "http://test.volansure.com");
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            if ((new_str = malloc(strlen(str1) + strlen(str2) + strlen(str3) + strlen(temperature2) + strlen(str4) + 20)) != NULL) {
                new_str[0] = '\0'; // ensures the memory is an empty string
                strcat(new_str, str1);
                strcat(new_str, "&");
                strcat(new_str, str2);
                strcat(new_str, "S2");
                strcat(new_str, "&");
                strcat(new_str, str3);
                strcat(new_str, temperature2);
                strcat(new_str, "&");
                strcat(new_str, str4);
                strcat(new_str, time_stamp());
            } else {
                fprintf(stderr, "malloc failed!\n");
                // exit?
            }
            //	    printf("%s", new_str);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, new_str);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {

                unqlite_util_random_string(pDb, zKey, sizeof (zKey)); // Generate key

                /* Store records */
                rc = unqlite_kv_store(pDb, zKey, sizeof (zKey), new_str, strlen(new_str));
                //printf("\n size display2: %s %lu", new_str, strlen(new_str));
                if (rc != UNQLITE_OK) {
                    /* Insertion fail, extract database error log and exit */
                    Fatal(pDb, 0);
                }
                unqlite_commit(pDb);
                count++;
                printf("\ncount: %d", count);

            }

            //	      fprintf(stderr, "curl_easy_perform() failed: %s\n",
            //              curl_easy_strerror(res));
            curl_easy_cleanup(curl);

        }
    }

    curl_global_cleanup();
    exit(EXIT_SUCCESS);
}
