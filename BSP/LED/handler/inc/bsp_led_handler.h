/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_led_handler.h
 * 
 * @par dependencies 
 * - stdio.h
 * - stdint.h
 * 
 * @author Jack | R&D Dept. | EternalChip Á¢Ð¾Ç¶ÈëÊ½
 * 
 * @brief Provides HAL APIs for LEDs and corresponding operations.
 * 
 * Processing flow:
 * 
 * Call directly.
 * 
 * @version V1.0 2024-10-18
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
#ifndef __BSP_LED_HANDLER_H__
#define __BSP_LED_HANDLER_H__

//******************************** Includes *********************************//
#include "bsp_led_driver.h"
#include <stdio.h>
#include <stdint.h>

//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define OS_SUPPORTING            /* OS_SUPPORTING depends on OS availability */
#define DEBUG                    /* Enable DEBUG                             */
#define DEBUG_OUT(X)   printf(X) /* DEBUG output info to indicate status     */

#define INIT_PATTERN  (bsp_led_driver_t *)(0xA6A6A6A6) 
                                 /* Init space with this pattern             */

typedef struct bsp_led_driver bsp_led_driver_t;

typedef enum
{
	HANDLER_NOT_INITED     = 1,      /* LED handler is not inited            */ 
    HANDLER_INITED         = 0,      /* LED handler is inited                */
} led_handler_init_t;

typedef enum
{
	LED_1                  = 0,      /* LED index number which passed to app */ 
    LED_2,                           /* LED index number which passed to app */
    LED_3,                           /* LED index number which passed to app */
    LED_4,                           /* LED index number which passed to app */
    LED_5,                           /* LED index number which passed to app */
    LED_6,                           /* LED index number which passed to app */
    LED_7,                           /* LED index number which passed to app */
    LED_8,                           /* LED index number which passed to app */
    LED_9,                           /* LED index number which passed to app */
    LED_10,                          /* LED index number which passed to app */
    MAX_INSTANCE_NUMBER,             /* LED index number which passed to app */ 
    LED_NOT_INITIALIZED = 0xFFFFFFFF /* LED index number which passed to app */
} led_index_t;

typedef enum
{
	HANDLER_OK             = 0,      /* LED operation completed successfully */
	HANDLER_ERROR          = 1,      /* LED runtime error without cause      */
	HANDLER_ERRORTIMEOUT   = 2,      /* LED operation failed due to timeout  */
	HANDLER_ERRORRESOURCE  = 3,      /* LED resource unavailable             */
	HANDLER_ERRORPARAMETER = 4,      /* LED parameter error                  */
	HANDLER_ERRORNOMEMORY  = 5,      /* LED out of memory                    */
	HANDLER_ERRORISR       = 6,      /* LED not allowed in ISR context       */
	HANDLER_RESERVED       = 0xFF,   /* Reserved for future use              */
} led_handler_status_t;

typedef struct
{
    led_handler_status_t (*pf_get_time_ms)  ( uint32_t * const );/* LED Op   */
} handler_time_base_ms_t;

#ifdef OS_SUPPORTING
typedef struct
{
    led_handler_status_t (*pf_os_delay_ms)  ( const   uint32_t );/* LED Op   */
} handler_os_delay_t;


typedef struct
{
    /* OS queue create */
    led_handler_status_t (*pf_os_queue_create )  ( 
                                                  uint32_t const     item_num,
                                                  uint32_t const    item_size,
                                                  void ** const queue_handler);
    
    /* OS queue put */
    led_handler_status_t (*pf_os_queue_put    )  ( 
                                                  void * const  queue_handler,
                                                  void * const           item,
                                                  uint32_t           timeout );
    
    /* OS queue get */                                        
    led_handler_status_t (*pf_os_queue_get    )  ( 
                                              void * const      queue_handler,
                                              void * const                msg,
                                              uint32_t               timeout );
    
    /* OS queue delete */                                        
    led_handler_status_t (*pf_os_queue_delete )  ( 
                                              void * const      queue_handler);
} handler_os_queue_t;

typedef struct 
{
    led_handler_status_t (*pf_os_critical_enter)  ( void ); /* LED Op   */
    led_handler_status_t (*pf_os_critical_exit )  ( void ); /* LED Op   */
}handler_os_critical_t;

typedef struct
{
    led_handler_status_t (*pf_os_thread_create ) (        /* Thread create          */
                void * const                   task_code, /* Defined internal*/
                const char * const             task_name, /* Defined external*/
                const uint32_t               stack_depth, /* Defined external*/
                void * const                  parameters, /* Defined internal*/
                uint32_t                        priority, /* Defined external*/
                void ** const               task_handler  /* Defined internal*/
                                     );

    led_handler_status_t (*pf_os_thread_delete ) (void *);/* Thread delete   */

} handler_os_thread_t;

typedef struct bsp_led_handler bsp_led_handler_t;

typedef led_handler_status_t (*pf_handler_led_control_t)(
                                         bsp_led_handler_t * const        self,
                                         uint32_t ,       //    Cycle time [ms]
                                         uint32_t ,       // Blink times[times]
                                         proportion_t,    // Proportion  on/off
                                         led_index_t         const        index
                                        );

//注册LED实例到handler中，并返回LED实例的index，index由handler分配，APP通过index来区分不同的LED实例
typedef led_handler_status_t (*pf_handler_led_register_t)(
                                         bsp_led_handler_t * const        self,
                                         bsp_led_driver_t  * const  led_driver,
                                         led_index_t        * const       index
                                        );
typedef struct
{
    uint32_t           led_instance_num;           /* Number of instances    */
    bsp_led_driver_t * led_instance_group[MAX_INSTANCE_NUMBER]; /* Array     */
} instance_registered_t;

typedef struct bsp_led_handler
{
    /**********Internal runtime data for internal status**********************/
    /**********Target of internal status**********/
    uint8_t                              is_inited;
    
    /**********Internal registered instances******/
    instance_registered_t                instances;   
    /*************Target of features**************/
    void *                           queue_handler;
    void *                          thread_handler;
    // May have more features later...

    /**********Interfaces for internal use***********************************/
    /*************Target of IOs needed************/
    /* Interface from Core layer                  */
    handler_time_base_ms_t         *p_time_base_ms;
    /* Interface from OS layer                    */
#ifdef OS_SUPPORTING
    os_delay_t                    *p_os_time_delay;
    handler_os_queue_t       *p_os_queue_interface;
    handler_os_critical_t           *p_os_critical;
    handler_os_thread_t               *p_os_thread;
#endif //OS_SUPPORTING

    /**********Interfaces for external use***********************************/
    /*************Target of APIs******************/
    /* For APP                                    */
    pf_handler_led_control_t     pf_led_countroler;
    /* For internal driver                        */
    pf_handler_led_register_t      pf_led_register;

}bsp_led_handler_t;



//******************************** Declaring ********************************//
/**
 * @brief Instantiates the bsp_led_handler_t target.
 * 
 * Steps:
 *  1. Adds Core interfaces into bsp_led_driver instance target.
 *  2. Adds OS interfaces into bsp_led_driver instance target.
 *  3. Adds timebase interfaces into bsp_led_driver instance target.
 *  
 * @param[in] self      : Pointer to the input data.
 * @param[in] led_ops   : Length of the input data.
 * @param[in] os_delay  : Pointer to the input data.
 * @param[in] time_base : Pointer to the input data.
 * 
 * @return led_handler_status_t : Status of the operation.
 * 
 * */
led_handler_status_t led_handler_inst (
                                  bsp_led_handler_t      * const        self, 
#ifdef OS_SUPPORTING 
                                  os_delay_t             * const    os_delay,
                                  handler_os_queue_t     * const    os_queue,
                                  handler_os_critical_t  * const os_critical,
                                  handler_os_thread_t    * const   os_thread,
#endif //OS_SUPPORTING 
                                  handler_time_base_ms_t * const  time_base  );

//******************************** Declaring ********************************//

#endif // End of __BSP_LED_HANDLER_H__

#endif