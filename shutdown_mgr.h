/*************************************************************
 *       Copyright 2016 Accton Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ************************************************************/

#ifndef __SHUTDOWN_MGR_H__
#define __SHUTDOWN_MGR_H__

int shutdown_mgr(void);
int shutdown_mgr_shutdown_linecard_local(int lc_slot_id);

int shutdown_mgr_power_linecard_local(int lc_slot_id, int option);

#endif /* __SHUTDOWN_MGR_H__ */
