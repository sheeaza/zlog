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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "zlog.h"

int main(int argc, char** argv)
{
	int rc;

	rc = dzlog_init("test_static_file_single.conf", "default");
	if (rc) {
		printf("init failed\n");
		return -1;
	}

    for (int i = 0; i < 1000000; i++) {
        dzlog_info("hello, zlog %d", i);
    }

	zlog_fini();
	
	return 0;
}
