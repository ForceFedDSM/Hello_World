/*
 *  Copyright 2016 HomeACcessoryKid - HacK - homeaccessorykid@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*****************************************************************************************
 * Welcome to the HomeACcessoryKid hkc button led
 * With a few lines of code we demonstrate the easy setup of your ESP8266 as an accessory.
 * Start defining your accessory in hkc_user_init and execute other pending init tasks.
 * For each Service characteristic a callback function is defined.
 * An ACC_callback will be called in different modes.
 * - mode=0: initialize your service (init)
 * - mode=1: a request for a change  is received on which you could act (write)
 * - mode=2: a request for a refresh is received where you might update  (read)
 * A callback should return QUICKLY, else use a Task as demonstrated below.
 *
 * If something changes from inside, you can use change_value and send_events in return.
 * You use aid and iid to know which characteristic to handle and cJSON for the value.
 *
 * Use iOS10 Home app or Eve or other app to test all the features and enjoy
*****************************************************************************************/
 
#include "esp_common.h"
#include "hkc.h"
#include "gpio.h"
#include "queue.h"

xQueueHandle identifyQueue;

struct  gpio {
    int aid;
    int iid;
  cJSON *value;
} gpio5;


/*void    led_intr()
{
    int             new;
    static uint32   oldtime;

    if ( (oldtime+200)<(oldtime=(system_get_time()/1000) ) ) {  //200ms debounce guard
        new=GPIO_INPUT(GPIO_Pin_2)^1; //get new state
        GPIO_OUTPUT(GPIO_Pin_2,new);       //toggle
        gpio2.value->type=new;
        change_value(    gpio2.aid,gpio2.iid,gpio2.value);
        send_events(NULL,gpio2.aid,gpio2.iid);
    }
}*/

void relay(int aid, int iid, cJSON *value, int mode)
{
    /*GPIO_ConfigTypeDef gpio0_in_cfg;*/
    GPIO_ConfigTypeDef gpio5_in_cfg;

    switch (mode) {
        case 1: { //changed by gui
            char *out; out=cJSON_Print(value);  os_printf("relay %s\n",out);  free(out);  // Print to text, print it, release the string.
            if (value) {
                     GPIO_OUTPUT(GPIO_Pin_5, value->type);
                     GPIO_OUTPUT(GPIO_Pin_2, value->type);
                       }
        }break;
        case 0: { //init
            //gpio0_in_cfg.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;         //Falling edge trigger
            //gpio0_in_cfg.GPIO_Mode     = GPIO_Mode_Input;               //Input mode
            //gpio0_in_cfg.GPIO_Pin      = GPIO_Pin_0;                    //Enable GPIO
            //gpio_config(&gpio0_in_cfg);                                 //Initialization function
            //gpio_intr_callbacks[0]=led_intr;                           //define the Pin0 callback
            
            gpio5_in_cfg.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;         //no interrupt
            gpio5_in_cfg.GPIO_Mode     = GPIO_Mode_Output;              //Output mode
            gpio5_in_cfg.GPIO_Pullup   = GPIO_PullUp_EN;                //improves transitions
            gpio5_in_cfg.GPIO_Pin      = GPIO_Pin_5;                    //Enable GPIO
            gpio_config(&gpio5_in_cfg);                                 //Initialization function
            
            relay(aid,iid,value,1);
            gpio5.aid=aid; gpio5.iid=iid;
            gpio5.value=cJSON_CreateBool(0); //value doesn't matter
        }break;
        case 2: { //update
            //do nothing
        }break;
        default: {
            //print an error?
        }break;
    }
}

void identify_task(void *arg)
{
    int i,original;

    os_printf("identify_task started\n");
    while(1) {
        while(!xQueueReceive(identifyQueue,NULL,10));//wait for a queue item
        original=GPIO_INPUT(GPIO_Pin_2); //get original state
        for (i=0;i<2;i++) {
            GPIO_OUTPUT(GPIO_Pin_2,original^1); // and toggle
            vTaskDelay(30); //0.3 sec
            GPIO_OUTPUT(GPIO_Pin_2,original^0);
            vTaskDelay(30); //0.3 sec
        }
    }
}

void identify(int aid, int iid, cJSON *value, int mode)
{
    switch (mode) {
        case 1: { //changed by gui
            xQueueSend(identifyQueue,NULL,0);
        }break;
        case 0: { //init
        identifyQueue = xQueueCreate( 1, 0 );
        xTaskCreate(identify_task,"identify",256,NULL,2,NULL);
        }break;
        case 2: { //update
            //do nothing
        }break;
        default: {
            //print an error?
        }break;
    }
}

extern  cJSON       *root;
void    hkc_user_init(char *accname)
{
    //do your init thing beyond the bear minimum
    //avoid doing it in user_init else no heap left for pairing
    cJSON *accs,*sers,*chas,*value;
    int aid=0,iid=0;

    accs=initAccessories();
    
    sers=addAccessory(accs,++aid);
    //service 0 describes the accessory
    chas=addService(      sers,++iid,APPLE,ACCESSORY_INFORMATION_S);
    addCharacteristic(chas,aid,++iid,APPLE,NAME_C,accname,NULL);
    addCharacteristic(chas,aid,++iid,APPLE,MANUFACTURER_C,"HacK",NULL);
    addCharacteristic(chas,aid,++iid,APPLE,MODEL_C,"Rev-1",NULL);
    addCharacteristic(chas,aid,++iid,APPLE,SERIAL_NUMBER_C,"1",NULL);
    addCharacteristic(chas,aid,++iid,APPLE,IDENTIFY_C,NULL,identify);
    //service 1
    chas=addService(      sers,++iid,APPLE,SWITCH_S);
    addCharacteristic(chas,aid,++iid,APPLE,NAME_C,"led",NULL);
    addCharacteristic(chas,aid,++iid,APPLE,POWER_STATE_C,"1",relay);

    char *out;
    out=cJSON_Print(root);  os_printf("%s\n",out);  free(out);  // Print to text, print it, release the string.

//  for (iid=1;iid<MAXITM+1;iid++) {
//      out=cJSON_Print(acc_items[iid].json);
//      os_printf("1.%d=%s\n",iid,out); free(out);
//  }

    //gpio_intr_handler_register(gpio_intr_handler,NULL);         //Register the interrupt function
    //GPIO_INTERRUPT_ENABLE;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{   
    os_printf("start of user_init @ %d\n",system_get_time()/1000);
    
//use this block only once to set your favorite access point or put your own selection routine
/*    wifi_set_opmode(STATION_MODE); 
    struct station_config *sconfig = (struct station_config *)zalloc(sizeof(struct station_config));
    sprintf(sconfig->ssid, ""); //don't forget to set this if you use it
    sprintf(sconfig->password, ""); //don't forget to set this if you use it
    wifi_station_set_config(sconfig);
    free(sconfig);
    wifi_station_connect(); /**/
    
    //try to only do the bare minimum here and do the rest in hkc_user_init
    // if not you could easily run out of stack space during pairing-setup
    hkc_init("button-led");
    
    os_printf("end of user_init @ %d\n",system_get_time()/1000);
}

/***********************************************************************************
 * FunctionName : user_rf_cal_sector_set forced upon us by espressif since RTOS1.4.2
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal    B : rf init data    C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
***********************************************************************************/
uint32 user_rf_cal_sector_set(void) {
    extern char flashchip;
    SpiFlashChip *flash = (SpiFlashChip*)(&flashchip + 4);
    // We know that sector size is 4096
    //uint32_t sec_num = flash->chip_size / flash->sector_size;
    uint32_t sec_num = flash->chip_size >> 12;
    return sec_num - 5;
}
