#include <stdio.h>  
#include <stdlib.h>

#include <unqlite.h>
#include <curl/curl.h>
#include <unistd.h>

/*
 * Banner.
 */
static const char zBanner[] = {
    "============================================================\n"
    "Volansure			                             \n"
    "                                         Retransmit data    \n"
    "============================================================\n"
};

static unqlite *pDb; /* Database handle */
static unqlite_kv_cursor *pCur; /* Cursor handle */
static int count, rc;

static void Fatal(unqlite *pDb, const char *zMsg) // Unqlite database routine for handling errors - not important
{
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

static int RestartRoutine() { // Routine to close and open the database again
    /* Release our cursor */
    unqlite_kv_cursor_release(pDb, pCur);
    unqlite_close(pDb);

    printf("\n Sleeping: ");
    printf("\n count: %d", count);

    sleep(10); // Sleep 10 seconds

    count = 0;
    /* Open our database */
    rc = unqlite_open(&pDb, "Volansure.db", UNQLITE_OPEN_CREATE);
    if (rc != UNQLITE_OK) {
        printf("\n\n Error in OPEN_CREATE");
        Fatal(0, "Out of memory");
        return 0;
    }

    /* Allocate a new cursor instance */
    rc = unqlite_kv_cursor_init(pDb, &pCur);
    if (rc != UNQLITE_OK) {
        printf("\n\n Error in new cursor");
        Fatal(0, "Out of memory");
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) // Main routine
{
    unqlite_int64 lData;
    int lKey, lData1;
    char *iKey, *iData;

    CURL *curl;

    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);

    count = 0;

    puts(zBanner);

    /* Open our database */
    rc = unqlite_open(&pDb, "Volansure.db", UNQLITE_OPEN_CREATE);
    if (rc != UNQLITE_OK) {
        Fatal(0, "Out of memory"); // Fatal may be replaced by restart routine to avoid contention due to lock by original thread.
    }

    /* Allocate a new cursor instance */
    rc = unqlite_kv_cursor_init(pDb, &pCur);
    if (rc != UNQLITE_OK) {
        Fatal(0, "Out of memory"); // Fatal may be replaced by restart routine to avoid contention due to lock by original thread.
    }

    while (1) { //infinite loop

        /* Point to the first record */
        unqlite_kv_cursor_first_entry(pCur);

        if (unqlite_kv_cursor_valid_entry(pCur) && count < 50) { // Release database for every 50 msgs

            /* Extract key length */
            unqlite_kv_cursor_key(pCur, NULL, &lKey);

            //Allocate a buffer big enough to hold the record content
            iKey = (char *) malloc(lKey);
            if (iKey == NULL) {
                printf("\n Problem in iKey");
                return;
            }

            /* Extract key */
            unqlite_kv_cursor_key(pCur, iKey, &lKey);
            printf("\n\nKey ==> %s", iKey);

            /* Extract data length */
            unqlite_kv_cursor_data(pCur, NULL, &lData);
            printf("\nData length ==> %lld", lData);

            lData1 = lData; // move to int from unsigned int
            printf("\nlData1 ==> %d", lData1);

            //Allocate a buffer big enough to hold the record content
            iData = (char *) malloc(lData1);
            if (iData == NULL) {
                printf("\n Problem in iData");
                return;
            }

            /* Extract data */
            unqlite_kv_cursor_data(pCur, iData, &lData);
            printf("\nData ==> %s", iData);

            curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, "http://test.volansure.com");
                curl_easy_setopt(curl, CURLOPT_POST, 1);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, iData);

                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    printf("\n Enterprise server is not up");
                    fprintf(stderr, "\n curl_easy_perform() failed: %s\n",
                            curl_easy_strerror(res));
                    curl_easy_cleanup(curl);
                    do {
                        rc = RestartRoutine();
                    } while (rc != 1);
                    continue;
                }

                /* Point to the row to be deleted */
                rc = unqlite_kv_cursor_seek(pCur, iKey, lKey, UNQLITE_CURSOR_MATCH_EXACT);
                if (rc == UNQLITE_OK) {
                    /* Delete entry */
                    rc = unqlite_kv_cursor_delete_entry(pCur);
                    if (rc != UNQLITE_OK) {
                        printf("\n Problem in delete ");
                        char *zBuf;
                        int iLen;
                        /* Extract database error log */
                        unqlite_config(pDb, UNQLITE_CONFIG_ERR_LOG, &zBuf, &iLen);
                        if (iLen > 0) {
                            puts(zBuf);
                        }
                        if (rc != UNQLITE_BUSY && rc != UNQLITE_NOTFOUND) {
                            /* Rollback */
                            printf("\n Delete rollback ");
                            unqlite_rollback(pDb);
                        }
                        return;
                    }
                }

                unqlite_commit(pDb); // commit after every delete

                count++;

                free(iKey);
                free(iData);
            }
        } else {
            do {
                rc = RestartRoutine();
            } while (rc != 1);
        }
    }

    /* Auto-commit the transaction and close our database */
    unqlite_close(pDb);
    return 0;
}
