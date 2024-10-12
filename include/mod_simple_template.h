/*
 * mod_opendata_projects.h
 *
 *  Created on: 12 Oct 2024
 *      Author: billy
 */

#ifndef INCLUDE_MOD_SIMPLE_TEMPLATE_H_
#define INCLUDE_MOD_SIMPLE_TEMPLATE_H_


/**
 * @brief The configuration for the Simple Template module.
 */
typedef struct
{
	/** The filename of the json records file */
	const char *mstc_records_s;

	/** The root url to determine the template page from */
	const char *mstc_root_url_s;


	/** The root directory containing the templates */
	const char *mstc_templates_dir_s;


} ModSimpleTemplateConfig;



#endif /* INCLUDE_MOD_SIMPLE_TEMPLATE_H_ */
