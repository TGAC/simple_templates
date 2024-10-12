

#include "mod_simple_template.h"

#include <string.h>


#include "apr_hash.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

#include "util_script.h"
#include "apr_escape.h"


#include "jansson.h"
#include "string_utils.h"
#include "byte_buffer.h"
#include "json_util.h"





/*
 * STATIC FUNCTION DECLARATIONS
 */

static int SimpleTemplateHandler (request_rec *req_p);

static void RegisterHooks (apr_pool_t *pool_p);


static void *MergeSimpleTemplateDirectoryConfig (apr_pool_t *pool_p, void *base_config_p, void *new_config_p);


static void *MergeSimpleTemplateServerConfig (apr_pool_t *pool_p, void *base_config_p, void *vhost_config_p);


static void *CreateSimpleTemplateServerConfig (apr_pool_t *pool_p, server_rec *server_p);


static void *CreateSimpleTemplateDirectoryConfig (apr_pool_t *pool_p, char *context_s);


static ModSimpleTemplateConfig *CreateConfig (apr_pool_t *pool_p, server_rec *server_p);


static const char *SetRecordsFile (cmd_parms *cmd_p, void *cfg_p, const char *arg_s);

static const char *SetRootURL (cmd_parms *cmd_p, void *cfg_p, const char *arg_s);

static const char *SetTemplatesDir (cmd_parms *cmd_p, void *cfg_p, const char *arg_s);

static const char *GetParameterValue (apr_table_t *params_p, const char * const param_s, apr_pool_t *pool_p);

/*
 * STATIC VARIABLES
 */

static const command_rec s_template_directives [] =
{
	AP_INIT_TAKE1 ("SimpleTemplateRecordsFile", SetRecordsFile, NULL, ACCESS_CONF, "The filename of the records file to use"),
	AP_INIT_TAKE1 ("SimpleTemplateRootURL", SetRootURL, NULL, ACCESS_CONF, "The root url to determine the template from"),
	AP_INIT_TAKE1 ("SimpleTemplateTemplatesDir", SetRootURL, NULL, ACCESS_CONF, "The root url to determine the template from"),

	{ NULL }
};

/* Define our module as an entity and assign a function for registering hooks  */
module AP_MODULE_DECLARE_DATA simple_template_module =
{
	STANDARD20_MODULE_STUFF,
	CreateSimpleTemplateDirectoryConfig,   	// Per-directory configuration handler
	MergeSimpleTemplateDirectoryConfig,   	// Merge handler for per-directory configurations
	CreateSimpleTemplateServerConfig,				// Per-server configuration handler
	MergeSimpleTemplateServerConfig,				// Merge handler for per-server configurations
	s_template_directives,			// Any directives we may have for httpd
	RegisterHooks    					// Our hook registering function
};




/* register_hooks: Adds a hook to the httpd process */
static void RegisterHooks (apr_pool_t *pool_p)
{
	ap_hook_handler (SimpleTemplateHandler, NULL, NULL, APR_HOOK_MIDDLE);
}


static const char *SetRecordsFile (cmd_parms *cmd_p, void *cfg_p, const char *arg_s)
{
	ModSimpleTemplateConfig *config_p = (ModSimpleTemplateConfig *) cfg_p;

	config_p -> mstc_records_s = arg_s;

	return NULL;
}


static const char *SetRootURL (cmd_parms *cmd_p, void *cfg_p, const char *arg_s)
{
	ModSimpleTemplateConfig *config_p = (ModSimpleTemplateConfig *) cfg_p;

	config_p -> mstc_root_url_s = arg_s;

	return NULL;
}


static const char *SetTemplatesDir (cmd_parms *cmd_p, void *cfg_p, const char *arg_s)
{
	ModSimpleTemplateConfig *config_p = (ModSimpleTemplateConfig *) cfg_p;

	config_p -> mstc_templates_dir_s = arg_s;

	return NULL;
}

static void *MergeSimpleTemplateDirectoryConfig (apr_pool_t *pool_p, void *base_config_p, void *new_config_p)
{
	ModSimpleTemplateConfig *base_template_config_p = (ModSimpleTemplateConfig *) base_config_p;
	ModSimpleTemplateConfig *new_template_config_p = (ModSimpleTemplateConfig *) new_config_p;

	if (new_template_config_p -> mstc_records_s)
		{
			base_template_config_p -> mstc_records_s = new_template_config_p -> mstc_records_s;
		}

	if (new_template_config_p -> mstc_root_url_s)
		{
			base_template_config_p -> mstc_root_url_s = new_template_config_p -> mstc_root_url_s;
		}

	if (new_template_config_p -> mstc_templates_dir_s)
		{
			base_template_config_p -> mstc_templates_dir_s = new_template_config_p -> mstc_templates_dir_s;
		}

	return base_config_p;
}



static void *MergeSimpleTemplateServerConfig (apr_pool_t *pool_p, void *base_config_p, void *vhost_config_p)
{
	/* currently ignore the vhosts config */
	return base_config_p;
}


static void *CreateSimpleTemplateServerConfig (apr_pool_t *pool_p, server_rec *server_p)
{
	return ((void *) CreateConfig (pool_p, server_p));
}


static void *CreateSimpleTemplateDirectoryConfig (apr_pool_t *pool_p, char *context_s)
{
	return ((void *) CreateConfig (pool_p, NULL));
}


static ModSimpleTemplateConfig *CreateConfig (apr_pool_t *pool_p, server_rec *server_p)
{
	ModSimpleTemplateConfig *config_p = apr_palloc (pool_p, sizeof (ModSimpleTemplateConfig));

	if (config_p)
		{
			config_p -> mstc_records_s = NULL;
			config_p -> mstc_root_url_s = NULL;
			config_p -> mstc_templates_dir_s = NULL;
		}

	return config_p;
}


/* The handler function for our module.
 * This is where all the fun happens!
 */
static int SimpleTemplateHandler (request_rec *req_p)
{
	int res = DECLINED;

	/* First off, we need to check if this is a call for the grassroots handler.
	 * If it is, we accept it and do our things, it not, we simply return DECLINED,
	 * and Apache will try somewhere else.
	 */
	if ((req_p -> handler) && (strcmp (req_p -> handler, "simple-template-handler") == 0))
		{
			if (req_p -> method_number == M_GET)
				{
					ModSimpleTemplateConfig *config_p = ap_get_module_config (req_p -> per_dir_config, &simple_template_module);

					char *api_call_s = req_p -> uri;

					if (api_call_s)
						{
							bool loop_flag = false;
							apr_table_t *params_p = NULL;

							/* jump to the rest api call string */
							if (config_p -> mstc_root_url_s)
								{
									api_call_s += strlen (config_p -> mstc_root_url_s);
								}

							ap_args_to_table (req_p, &params_p);


							while (loop_flag)
								{

								}
						}

				}		/* if (req_p -> method_number == M_GET) */


		}		/* if ((req_p -> handler) && (strcmp (req_p -> handler, "brapi-handler") == 0)) */

	return res;
}



/*

  Open the json file
  
  find the required project by uuid 
  
  load the required output template
  
  replace the keys within the template by the project's values
 */
 
static const json_t *GetRecordByKey (const json_t *records_p, const char * const key_s, const char * const search_s);

static char * RunModule (const char *records_file_s, const char * const key_s, const char * const search_s, const char *const template_filename_s);


static char * RunModule (const char *records_file_s, const char * const key_s, const char * const search_s, const char *const template_filename_s)
{
	char *result_s = NULL;
	json_error_t err;
	json_t *records_p = json_load_file (records_file_s, 0, &err);

	if (records_p)
		{
			const json_t *record_p = GetRecordByKey (records_p, key_s, search_s);

			if (record_p)
				{
					char *template_s = GetFileContentsAsStringByFilename (template_filename_s);

					if (template_s)
						{
							/* Parse template and replace tokens */
							size_t template_size = strlen (template_s);

							ByteBuffer *buffer_p = AllocateByteBuffer (template_size << 1);

							if (buffer_p)
								{
									char *cursor_p = template_s;
									char *start_p = NULL;
									const char * const start_tag_s = "${";
									const char * const end_tag_s = "${";
									const size_t start_tag_length = strlen (start_tag_s);

									while ((start_p = strstr (cursor_p, start_tag_s)) != NULL)
										{
											char *end_p = strstr (start_p + start_tag_length, end_tag_s);

											if (end_p)
												{
													 if (AppendToByteBuffer (buffer_p, cursor_p, start_p - cursor_p))
														 {
															 char *variable_s = NULL;

															 start_p += start_tag_length;
															 variable_s = CopyToNewString (start_p, end_p - 1 - start_p, false);

															 if (variable_s)
																 {
																	 const json_t *value_p = json_object_get (record_p, variable_s);

																	 if (value_p)
																		 {
																			 if (json_is_string (value_p))
																				 {
																					 const char *value_s = json_string_value (value_p);

																					 if (AppendStringToByteBuffer (buffer_p, value_s))
																						 {

																						 }
																				 }
																		 }
																 }

															 cursor_p = end_p + 1;
														 }
												}
										}

									if (!AppendStringToByteBuffer (buffer_p, cursor_p))
										{
											/* Failed to append remaining data to buffer */
										}

									result_s = DetachByteBufferData (buffer_p);
								}		/* if (buffer_p) */

						}		/* if (template_s) */

				}		/* if (record_p) */

			json_decref (records_p);
		}		/* if (records_p) */

	return result_s;
}



static const json_t *GetRecordByKey (const json_t *records_p, const char * const key_s, const char * const search_s)
{
	size_t i;
	const size_t num_records = json_array_size (records_p);
	
	for (i = 0; i < num_records; ++ i)
		{
			const json_t *record_p = json_array_get (records_p, i);
			const char *value_s = GetJSONString (record_p, key_s);
			
			if (value_s)
				{
					if (strcmp (value_s, search_s) == 0)
						{
							return record_p;
						}				
				}
		
		}
 
 return NULL;
}
 


static const char *GetParameterValue (apr_table_t *params_p, const char * const param_s, apr_pool_t *pool_p)
{
	const char *value_s = NULL;
	const char * const raw_value_s = apr_table_get (params_p, param_s);
	const char *forbid_s = NULL;
	const char *reserved_s = NULL;

	if (raw_value_s)
		{
			value_s = apr_punescape_url (pool_p, raw_value_s, forbid_s, reserved_s, 1);
		}

	return value_s;
}

