/**********************************************************************
* - Description:		esp32-mpu6050
* - File:				i2c_application.c
* - Compiler:			xtensa-esp32
* - Debugger:			USB2USART
* - Author:				Mohamed El-Sabagh
* - Target:				ESP32
* - Created:			2017-12-11
* - Last changed:		2017-12-11
*
**********************************************************************/
#include "i2c_app.h"
#include "i2c_driver.h"
#include <esp_log.h>


#define I2C_APP_QUEUE_LENGTH            1
#define I2C_APP_WRITE_ITEM_SIZE         sizeof( I2C_Status )
#define I2C_APP_READ_ITEM_SIZE          sizeof( I2C_Status )

#define GUARD_CYCLE		10

static const char *TAG = "I2c_App";

/* Extern Variables	*/
/* Declare a variable of type xQueueHandle.  This is used to store the queue
	that is accessed by any task wants to write to I2C. */
QueueHandle_t xQueueI2CWriteBuffer;
QueueHandle_t xQueueI2CReadBuffer;

SemaphoreHandle_t xBinarySemaphoreI2CAppEndOfWrite;
SemaphoreHandle_t xBinarySemaphoreI2CAppEndOfRead;


/**
  * @brief  Write to I2C.
  * @param  None
  * @retval None
  */
void vI2CWrite( void *pvParameters )
{
	/* Declare the variable that will hold the values received from the queue. */
	I2C_Status lReceivedValue;
	portBASE_TYPE xStatus;

	/* The queue is created xStaticReadQueueto hold a maximum of 1 structure entry. */
	xQueueI2CWriteBuffer = xQueueCreate(I2C_APP_QUEUE_LENGTH, I2C_APP_WRITE_ITEM_SIZE);

	/* Create a binary semaphore without using any dynamic memory
	allocation.  The semaphore's data structures will be saved into
	the xSemaphoreWriteBuffer variable. */
	xBinarySemaphoreI2CAppEndOfWrite = xSemaphoreCreateBinary();

	/* The pxSemaphoreBuffer was not NULL, so it is expected that the
	handle will not be NULL. */
	configASSERT( xBinarySemaphoreI2CAppEndOfWrite );

	/* Rest of the task code goes here. */

	/* Check the semaphore was created successfully. */
	if( xBinarySemaphoreI2CAppEndOfWrite != NULL )
	{
		/* As per most tasks, this task is implemented within an infinite loop.
			Take the semaphore once to start with so the semaphore is empty before the
			infinite loop is entered.  The semaphore was created before the scheduler
			was started so before this task ran for the first time.*/
		xSemaphoreTake( xBinarySemaphoreI2CAppEndOfWrite, 0 );
	}

	for(;;)
	{
		/* The first parameter is the queue from which data is to be received..

		The second parameter is the buffer into which the received data will be
		placed.  In this case the buffer is simply the address of a variable that
		has the required size to hold the received data.

		the last parameter is the block time ? the maximum amount of time that the
		task should remain in the Blocked state to wait for data to be available should
		the queue already be empty. */
		xStatus = xQueueReceive( xQueueI2CWriteBuffer, (void *)&lReceivedValue, portMAX_DELAY );
		if( xStatus == pdPASS )
		{
			/* Write on I2C */
			i2c_write_bytes(lReceivedValue.device_address, lReceivedValue.register_address, lReceivedValue.size ,lReceivedValue.data);

			vTaskDelay( GUARD_CYCLE / portTICK_RATE_MS );

			/* 'Give' the semaphore to unblock the task. */
			xSemaphoreGive( xBinarySemaphoreI2CAppEndOfWrite );
		}
		else
		{
			/* We did not receive anything from the queue even after waiting for blocking time defined.*/
		}
	}

	vTaskDelete(NULL);
}

/**
  * @brief  Read from I2C.
  * @param  None
  * @retval None
  */
void vI2CRead( void *pvParameters )
{
	/* Declare the variable that will hold the values received from the queue. */
	I2C_Status lReceivedValue;
	portBASE_TYPE xStatus;

	/* The queue is created to hold a maximum of 1 structure entry. */
	xQueueI2CReadBuffer = xQueueCreate(I2C_APP_QUEUE_LENGTH, I2C_APP_READ_ITEM_SIZE);

	/* Create a binary semaphore without using any dynamic memory
	allocation.  The semaphore's data structures will be saved into
	the xSemaphoreReadBuffer variable. */
	xBinarySemaphoreI2CAppEndOfRead = xSemaphoreCreateBinary();

	/* The pxSemaphoreBuffer was not NULL, so it is expected that the
	handle will not be NULL. */
	configASSERT( xBinarySemaphoreI2CAppEndOfRead );

	/* Rest of the task code goes here. */

	/* Check the semaphore was created successfully. */
	if( xBinarySemaphoreI2CAppEndOfRead != NULL )
	{
		/* As per most tasks, this task is implemented within an infinite loop.
			Take the semaphore once to start with so the semaphore is empty before the
			infinite loop is entered.  The semaphore was created before the scheduler
			was started so before this task ran for the first time.*/
		xSemaphoreTake( xBinarySemaphoreI2CAppEndOfRead, 0 );
	}

	for(;;)
	{
		/* The first parameter is the queue from which data is to be received..

		The second parameter is the buffer into which the received data will be
		placed.  In this case the buffer is simply the address of a variable that
		has the required size to hold the received data.

		the last parameter is the block time ? the maximum amount of time that the
		task should remain in the Blocked state to wait for data to be available should
		the queue already be empty. */
		xStatus = xQueueReceive( xQueueI2CReadBuffer, (void *)&lReceivedValue, portMAX_DELAY );
		if( xStatus == pdPASS )
		{
			/* Read from I2C */
			i2c_read_bytes(lReceivedValue.device_address, lReceivedValue.register_address, lReceivedValue.size , lReceivedValue.data);

			vTaskDelay( GUARD_CYCLE / portTICK_RATE_MS );

			/* 'Give' the semaphore to unblock the task. */
			xSemaphoreGive( xBinarySemaphoreI2CAppEndOfRead );
		}
		else
		{
			/* We did not receive anything from the queue even after waiting for blocking time defined.*/
		}
	}

	vTaskDelete(NULL);
}


