#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include "view/view.h"
#include "model/model.h"


void controller_init(mut_model_t *model);
void controller_manage(mut_model_t *model);
void controller_sync_minion(model_t *model);



#endif
