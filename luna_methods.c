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

#define ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-"

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
    // %%% IGNORING RETURN ALERT %%%
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

bool start_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = true;
  char line[MAXLINELEN];
  // %%% MAGIC NUMBERS ALERT %%%
  char name[128];
  char status[128];

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonResponse = 0;
  int len = 0;

  json_t *object = LSMessageGetPayloadJSON(message);

  json_t *id = json_find_first_label(object, "id");               
  if (strspn(id->child->text, ALLOWED_CHARS) != strlen(id->child->text)) {
    LSMessageReply(lshandle, message, "{\"returnValue\":false,\"errorCode\":-1,\"errorText\":\"Invalid id\"}", &lserror);
    LSErrorFree(&lserror);
    return true;
  }

  // %%% MAGIC NUMBERS ALERT %%%
  char command[128];
  char format[128];

  // %%% IGNORING RETURN ALERT %%%
  sprintf((char *)&command, "/sbin/initctl start %s 2>&1", id->child->text);

  json_t *response = json_new_object();

  FILE *fp = popen(command, "r");
  if (fp) {
    while ( fgets( line, sizeof line, fp)) {
      if (sscanf(line, "(%*d/%*d) Job not changed: %127s\n", (char *)&name) == 1) {
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(response, "status", json_new_string("Job not changed"));
      }
      else if (sscanf(line, "(%*d/%*d) %s %127c\n", (char *)&name, (char *)&status) == 2) {
	// %%% HACK ALERT %%%
	*strchr(status,'\n') = 0;
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(response, "status", json_new_string(status));
      }
    }
    if (!pclose(fp)) {
      // %%% IGNORING RETURN ALERT %%%
      json_insert_pair_into_object(response, "returnValue", json_new_true());
    }
    else {
      // %%% IGNORING RETURN ALERT %%%
      json_insert_pair_into_object(response, "returnValue", json_new_false());
    }
    json_tree_to_string(response, &jsonResponse);
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

bool stop_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = true;
  char line[MAXLINELEN];
  // %%% MAGIC NUMBERS ALERT %%%
  char name[128];
  char status[128];

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonResponse = 0;
  int len = 0;

  json_t *object = LSMessageGetPayloadJSON(message);

  json_t *id = json_find_first_label(object, "id");               
  if (strspn(id->child->text, ALLOWED_CHARS) != strlen(id->child->text)) {
    LSMessageReply(lshandle, message, "{\"returnValue\":false,\"errorCode\":-1,\"errorText\":\"Invalid id\"}", &lserror);
    LSErrorFree(&lserror);
    return true;
  }

  // %%% MAGIC NUMBERS ALERT %%%
  char command[128];
  char format[128];

  // %%% IGNORING RETURN ALERT %%%
  sprintf((char *)&command, "/sbin/initctl stop %s 2>&1", id->child->text);

  json_t *response = json_new_object();

  FILE *fp = popen(command, "r");
  if (fp) {
    while ( fgets( line, sizeof line, fp)) {
      if (sscanf(line, "(%*d/%*d) Job not changed: %127s\n", (char *)&name) == 1) {
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(response, "status", json_new_string("Job not changed"));
      }
      else if (sscanf(line, "(%*d/%*d) %s %127c\n", (char *)&name, (char *)&status) == 2) {
	// %%% HACK ALERT %%%
	*strchr(status,'\n') = 0;
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(response, "status", json_new_string(status));
      }
    }
    if (!pclose(fp)) {
      // %%% IGNORING RETURN ALERT %%%
      json_insert_pair_into_object(response, "returnValue", json_new_true());
    }
    else {
      // %%% IGNORING RETURN ALERT %%%
      json_insert_pair_into_object(response, "returnValue", json_new_false());
    }
    json_tree_to_string(response, &jsonResponse);
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

bool jps_method(LSHandle* lshandle, LSMessage *message, void *ctx) {

  bool returnVal = true;
  char line[MAXLINELEN];
  // %%% MAGIC NUMBERS ALERT %%%
  char name[128];

  LSError lserror;
  LSErrorInit(&lserror);

  char *jsonResponse = 0;
  int len = 0;

  json_t *response = json_new_object();

  FILE *fp = popen("/usr/bin/jps", "r");
  if (fp) {
    json_t *array = json_new_array();
    // Skip the first line
    (void)fgets( line, sizeof line, fp);
    while ( fgets( line, sizeof line, fp)) {
      if (sscanf(line, "%*d %*d %*d %*d %*d %*d %*d %127c",
		 (char*)&name) == 1) {
	// %%% HACK ALERT %%%
	*strchr(name,'\n') = 0;
	json_t *object = json_new_object();
	// %%% IGNORING RETURN ALERT %%%
	json_insert_pair_into_object(object, "name", json_new_string(name));
	json_insert_child(array, object);
      }
    }
    if (!pclose(fp)) {
      // %%% IGNORING RETURN ALERT %%%
      json_insert_pair_into_object(response, "returnValue", json_new_true());
      json_insert_pair_into_object(response, "threads", array);
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

LSMethod luna_methods[] = {
  { "version",	version_method },
  { "list",	list_method },
  { "start",	start_method },
  { "stop",	stop_method },
  { "status",	dummy_method },
  { "jobs",	dummy_method },
  { "emit",	dummy_method },
  { "events",	dummy_method },
  { "respawn",	dummy_method },
  { "jps",	jps_method },
  { 0, 0 }
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
  return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
				       NULL, NULL, NULL, &lserror);
}
