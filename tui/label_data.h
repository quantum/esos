/*
 * $Id: label_data.h 139 2012-07-24 20:17:34Z marc.smith $
 */

#ifndef _LABEL_DATA_H
#define	_LABEL_DATA_H

#ifdef	__cplusplus
extern "C" {
#endif

#include"main.h"

void readAdapterData(char *label_msg[]);
void readDeviceData(char *label_msg[]);
void readTargetData(char *label_msg[]);

#ifdef	__cplusplus
}
#endif

#endif	/* _LABEL_DATA_H */

