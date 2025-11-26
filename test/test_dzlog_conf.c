/* Copyright (c) Hardy Simpson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zlog.h"

int main(int argc, char **argv)
{
    int rc;
    int opt;
    char *filename = NULL;

    static const struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0},
    };

    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "f:", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'f':
            filename = optarg;
            break;
        case '?': // Unknown option
            fprintf(stderr, "Usage: %s [-f <file>] [arguments...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!filename) {
        fprintf(stderr, "missing conf file\n");
        exit(EXIT_FAILURE);
    }

    if (access(filename, F_OK)) {
        fprintf(stderr, "invalid conf file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    rc = dzlog_init(filename, "default");
    if (rc) {
        printf("init failed\n");
        return -1;
    }

    for (int i = 0; i < 50; i++) {
        dzlog_info("hello, zlog %d", i);
    }

    zlog_fini();

    return 0;
}
