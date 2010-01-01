/*=============================================================================
 Copyright (C) 2009 Ryan Hope <rmh3093@gmail.com>
 Copyright (C) 2010 WebOS Internals <support@webos-internals.org>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "luna_service.h"
#include "luna_methods.h"

bool dummy_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = true;

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonResponse = 0;
  int len = 0;

  len = asprintf(&jsonResponse, "{\"returnValue\":true}", VERSION);
  if (jsonResponse) {
    LSMessageReply(lshandle, message, jsonResponse, &lserror);
    free(jsonResponse);
  } else
    LSMessageReply(lshandle, message, "{\"returnValue\":false,\"errorCode\":-1,\"errorText\":\"Generic error\"}", &lserror);

  LSErrorFree(&lserror);

  return returnVal;
}

bool version_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = true;
  char line[MAXLINELEN];
  // %%% MAGIC NUMBER ALERT %%%
  char version[16];

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonResponse = 0;
  int len = 0;

  FILE *fp = popen("/sbin/initctl version", "r");
  if (fp) {
    while ( fgets( line, sizeof line, fp)) {
      // %%% MAGIC NUMBER ALERT %%%
      if (sscanf(line, "(%*d/%*d) upstart %15s\n", (char*)&version) == 1) {
	len = asprintf(&jsonResponse, "{\"returnValue\":true,\"version\":\"%s\"}", version);
      }
    }
    pclose(fp);
  }

  if (jsonResponse) {
    LSMessageReply(lshandle, message, jsonResponse, &lserror);
    free(jsonResponse);
  } else
    LSMessageReply(lshandle, message, "{\"returnValue\":false,\"errorCode\":-1,\"errorText\":\"Generic error\"}", &lserror);
 
  LSErrorFree(&lserror);

  return returnVal;
}

bool list_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = true;
  char line[MAXLINELEN];
  // %%% MAGIC NUMBERS ALERT %%%
  char name[128];
  char state[16];
  char status[128];

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonResponse = 0;
  int len = 0;

  json_t *response = json_new_object();

  FILE *fp = popen("/sbin/initctl list", "r");
  if (fp) {
    json_t *array = json_new_array();
    while ( fgets( line, sizeof line, fp)) {
      fprintf(stderr, "line: %s", line);
      // %%% MAGIC NUMBERS ALERT %%%
      if (sscanf(line, "(%*d/%*d) %127s (start) %127c",
		 (char*)&name, (char *)&status) == 2) {
	// %%% HACK ALERT %%%
	*strchr(status,'\n') = 0;
	json_t *object = json_new_object();
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(object, "name", json_new_string(name));
	json_insert_pair_into_object(object, "state", json_new_string("start"));
	json_insert_pair_into_object(object, "status", json_new_string(status));
	json_insert_child(array, object);
      }
      // %%% MAGIC NUMBERS ALERT %%%
      else if (sscanf(line, "(%*d/%*d) %127s (stop) %127c",
		 (char*)&name, (char *)&status) == 2) {
	// %%% HACK ALERT %%%
	*strchr(status,'\n') = 0;
	json_t *object = json_new_object();
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(object, "name", json_new_string(name));
	json_insert_pair_into_object(object, "state", json_new_string("stop"));
	json_insert_pair_into_object(object, "status", json_new_string(status));
	json_insert_child(array, object);
      }
    }
    if (!pclose(fp)) {
      // %%% IGNORING RETURN ALERT %%%
      json_insert_pair_into_object(response, "returnValue", json_new_true());
      json_insert_pair_into_object(response, "jobs", array);
      json_tree_to_string(response, &jsonResponse);
    }
  }

  if (jsonResponse) {
    LSMessageReply(lshandle, message, jsonResponse, &lserror);
    free(jsonResponse);
  } else
    LSMessageReply(lshandle, message, "{\"returnValue\":false,\"errorCode\":-1,\"errorText\":\"Generic error\"}", &lserror);
 
  json_free_value(&response);
  LSErrorFree(&lserror);

  return returnVal;
}

bool other_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = false;

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonArgs = 0;
  int len = 0;

  json_t *object = LSMessageGetPayloadJSON(message);

  // json_t *id = json_find_first_label(object, "id");               
  // json_t *params = json_find_first_label(object, "params");               

  if (json_tree_to_string(object, &jsonArgs)) {
    returnVal = LSCallOneReply(lshandle, "palm://com.palm.applicationManager/launch",
			       jsonArgs, NULL, NULL, NULL, &lserror);
    free(jsonArgs);
  }

  if (returnVal) {
    LSMessageReply(lshandle, message, "{\"returnValue\":true}", &lserror);
  } else
    LSMessageReply(lshandle, message, "{\"returnValue\":false,\"errorCode\":-1,\"errorText\":\"Generic error\"}", &lserror);

  LSErrorFree(&lserror);

  return returnVal;
}

LSMethod luna_methods[] = {
  { "version",	version_method },
  { "list",	list_method },
  { "start",	dummy_method },
  { "stop",	dummy_method },
  { "status",	dummy_method },
  { "jobs",	dummy_method },
  { "emit",	dummy_method },
  { "events",	dummy_method },
  { "respawn",	dummy_method },
  { 0, 0 }
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
  return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
				       NULL, NULL, NULL, &lserror);
}
