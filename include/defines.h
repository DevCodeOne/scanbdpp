#pragma once

// TODO correct paths for pipe and pid file

#define PIPE_PATH "scanbd.pipe"

#define SANE_REINIT_TIMEOUT 3

#define C_FROM_VALUE "from-value"
#define C_FROM_VALUE_DEF_INT 0
#define C_FROM_VALUE_DEF_STR ""

#define C_TO_VALUE "to-value"
#define C_TO_VALUE_DEF_INT 1
#define C_TO_VALUE_DEF_STR ".+"

#define C_FILTER "filter"

#define C_NUMERICAL_TRIGGER "numerical-trigger"

#define C_STRING_TRIGGER "string-trigger"

#define C_DESC "desc"
#define C_DESC_DEF "The description goes here"

#define C_SCRIPT "script"
#define C_SCRIPT_DEF ""

#define C_ENV "env"
#define C_ENV_FUNCTION "SCANBD_FUNCTION"
#define C_ENV_FUNCTION_DEF "SCANBD_FUNCTION"

#define C_ENV_DEVICE "device"
#define C_ENV_DEVICE_DEF "SCANBD_DEVICE"

#define C_ENV_ACTION "action"
#define C_ENV_ACTION_DEF "SCANBD_ACTION"

#define C_DEBUG "debug"
#define C_DEBUG_DEF false

#define C_MULTIPLE_ACTIONS "multiple_actions"
#define C_MULTIPLE_ACTIONS_DEF true

#define C_DEBUG_LEVEL "debug-level"
#define C_DEBUG_LEVEL_DEF 1

#define C_USER "user"
#define C_USER_DEF "saned"

#define C_GROUP "group"
#define C_GROUP_DEF "scanner"

#define C_SANED "saned"
#ifdef SANED_PATH
#define C_SANED_DEF SANED_PATH
#else
#error SANED_PATH is not set!
#endif

#define C_SANED_OPTS "saned_opt"
#define C_SANED_OPTS_DEF "{}"

#define C_SCRIPT_DIR "scriptdir"
#define C_SCRIPT_DIR_DEF ""

#define C_DEVICE_INSERT_SCRIPT "device_insert_script"
#define C_DEVICE_INSERT_SCRIPT_DEF ""
#define C_DEVICE_REMOVE_SCRIPT "device_remove_script"
#define C_DEVICE_REMOVE_SCRIPT_DEF ""

#define C_SANED_ENVS "saned_env"
#define C_SANED_ENVS_DEF "{}"

#define C_TIMEOUT "timeout"
#define C_TIMEOUT_DEF 500

#define C_PIDFILE "pidfile"
#define C_PIDFILE_DEF "scanbd.pid"

#define C_ENVIRONMENT "environment"

#define C_FUNCTION "function"
#define C_FUNCTION_DEF "^function.*"

#define C_ACTION "action"
#define C_ACTION_DEF "^scan.*"

#define C_GLOBAL "global"
#define C_DEVICE "device"

#define C_INCLUDE "include"

#define SCANBD_NULL_STRING "(null)"

#ifdef SCANBD_CFG_DIR
#define SCANBD_CONF SCANBD_CFG_DIR "/scanbd.conf"
#else
#error SCANBD_CFG_DIR is not set!
#endif

#define NAME_POLLING_MODE "scanbd"
#define NAME_MANAGER_MODE "scanbm"
