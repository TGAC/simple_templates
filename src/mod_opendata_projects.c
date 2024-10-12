
#include <string.h>

#include "jansson.h"
#include "string_utils.h"


/*

  Open the json file
  
  find the required project by uuid 
  
  load the required output template
  
  replace the keys within the template by the project's values
 */
 
 

static char * RunModule (const char *records_file_s, const char * const key_s, const char * const search_s, const char *const template_filename_s)
{
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
							void *itr_p = json_object_iter (record_p);

							while (itr_p)
								{
									const char *record_key_s = json_object_iter_key (itr_p);
									json_t *record_value_p = json_object_iter_value (itr_p);
									char *updated_template_s = NULL;
									const char *record_value_s = NULL;

									if (json_is_string (record_value_p))
										{
											record_value_s = json_string_value (record_value_p);
										}

									if (record_value_s)
										{
											if (SearchAndReplaceInString (template_s, &updated_template_s, record_key_s, record_value_s))
												{
													FreeCopiedString (template_s);
													template_s = updated_template_s;

													itr_p = json_object_iter_next (record_p, itr_p);
												}
											else
												{
													itr_p = NULL;
												}

										}


								}


						}		/* if (template_s) */

				}		/* if (record_p) */

			json_decref (records_p);
		}		/* if (records_p) */

	return NULL;
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
 
