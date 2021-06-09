/*
 * Copyright (c) 2015-2020 ACOINFO Co., Ltd.
 * All rights reserved.
 *
 * Detailed license information can be found in the LICENSE file.
 *
 * File: sddc_connector_example.c SDDC message server example.
 *
 * Author: Jiao.jinxing <jiaojinxing@acoinfo.com>
 *
 */

#include <ms_rtos.h>
#include "sddc.h"
#include "cJSON.h"

static void iot_pi_get_pic_thread(void *arg)
{
    sddc_connector_t *conn = arg;
    sddc_bool_t finish;
    void *data;
    ssize_t ret;
    size_t totol_len = 0;

    while (1) {
        ret = sddc_connector_get(conn, &data, &finish);
        if (ret < 0) {
            ms_printf("Failed to get!!\n");
            break;
        } else {
            ms_printf("Get %d byte\n", ret);
            totol_len += ret;
            if (finish) {
                break;
            }
        }
    }

    ms_printf("Total get %d byte\n", totol_len);

    sddc_connector_destroy(conn);
}

#define PICTURE_DATA    (void *)0x8040000
#define PICTURE_SIZE    1024

static void iot_pi_put_pic_thread(void *arg)
{
    sddc_connector_t *conn = arg;
    int ret;
    int i;
    size_t totol_len = 0;

    for (i = 0; i < 32; i++) {
        ret = sddc_connector_put(conn, PICTURE_DATA, PICTURE_SIZE - i, SDDC_FALSE);
        if (ret < 0) {
            ms_printf("Failed to put!!\n");
            break;
        }
        totol_len += PICTURE_SIZE - i;
        ms_printf("Put %d byte\n", PICTURE_SIZE - i);
    }

    ret = sddc_connector_put(conn, PICTURE_DATA, PICTURE_SIZE - i, SDDC_TRUE);
    totol_len += PICTURE_SIZE - i;
    ms_printf("Put %d byte\n", PICTURE_SIZE - i);

    ms_printf("Total put %d byte\n", totol_len);

    sddc_connector_destroy(conn);
}

/*
 * handle MESSAGE
 */
static sddc_bool_t iot_pi_on_message(sddc_t *sddc, const uint8_t *uid, const char *message, size_t len)
{
    cJSON *root = cJSON_Parse(message);

    char *str = cJSON_Print(root);
    ms_printf("iot_pi_on_message: %s\n", str);
    cJSON_free(str);

    /*
     * Parse here
     */
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    if (cJSON_IsString(cmd)) {
        sddc_bool_t get_mode;

        if (strcmp(cmd->valuestring, "recv") == 0) {
            get_mode = SDDC_FALSE;
        } else if (strcmp(cmd->valuestring, "send") == 0) {
            get_mode = SDDC_TRUE;

            cJSON *size = cJSON_GetObjectItem(root, "size");
            if (size && cJSON_IsNumber(size)) {
                ms_printf("EdgerOS send picture to me, file size %d\n", (int)size->valuedouble);
            }

        } else {
            ms_printf("command no support!\n");
            goto error;
        }


        cJSON *connector = cJSON_GetObjectItem(root, "connector");
        sddc_goto_error_if_fail(cJSON_IsObject(connector));

        cJSON *port = cJSON_GetObjectItem(connector, "port");
        sddc_goto_error_if_fail(cJSON_IsNumber(port));

        cJSON *token = cJSON_GetObjectItem(connector, "token");
        sddc_goto_error_if_fail(!token || cJSON_IsString(token));

        sddc_connector_t *conn = sddc_connector_create(sddc, uid, port->valuedouble, token ? token->valuestring : NULL, get_mode);
        sddc_goto_error_if_fail(conn);

        if (get_mode) {
            ms_thread_create("t_get_pic", (ms_thread_entry_t)iot_pi_get_pic_thread, conn,
                             4096, 9, 0, MS_THREAD_OPT_USER | MS_THREAD_OPT_REENT_EN, MS_NULL);
        } else {
            ms_thread_create("t_put_pic", (ms_thread_entry_t)iot_pi_put_pic_thread, conn,
                             4096, 9, 0, MS_THREAD_OPT_USER | MS_THREAD_OPT_REENT_EN, MS_NULL);
        }
    } else {
        ms_printf("command no specify!\n");
    }

error:
    cJSON_Delete(root);

    return SDDC_TRUE;
}

/*
 * handle MESSAGE ACK
 */
static void iot_pi_on_message_ack(sddc_t *sddc, const uint8_t *uid, uint16_t seqno)
{

}

/*
 * handle MESSAGE lost
 */
static void iot_pi_on_message_lost(sddc_t *sddc, const uint8_t *uid, uint16_t seqno)
{

}

/*
 * handle EdgerOS lost
 */
static void iot_pi_on_edgeros_lost(sddc_t *sddc, const uint8_t *uid)
{

}

/*
 * handle UPDATE
 */
static sddc_bool_t iot_pi_on_update(sddc_t *sddc, const uint8_t *uid, const char *udpate_data, size_t len)
{
    cJSON *root = cJSON_Parse(udpate_data);

    if (root) {
        /*
         * Parse here
         */

        char *str = cJSON_Print(root);

        ms_printf("iot_pi_on_update: %s\n", str);

        cJSON_free(str);

        cJSON_Delete(root);

        return SDDC_TRUE;
    } else {
        return SDDC_FALSE;
    }
}

/*
 * handle INVITE
 */
static sddc_bool_t iot_pi_on_invite(sddc_t *sddc, const uint8_t *uid, const char *invite_data, size_t len)
{
    cJSON *root = cJSON_Parse(invite_data);

    if (root) {
        /*
         * Parse here
         */

        char *str = cJSON_Print(root);

        ms_printf("iot_pi_on_invite: %s\n", str);

        cJSON_free(str);

        cJSON_Delete(root);

        return SDDC_TRUE;
    } else {
        return SDDC_FALSE;
    }
}

/*
 * handle the end of INVITE
 */
static sddc_bool_t iot_pi_on_invite_end(sddc_t *sddc, const uint8_t *uid)
{
    return SDDC_TRUE;
}

/*
 * Create REPORT data
 */
static char *iot_pi_report_data_create(void)
{
    cJSON *root;
    cJSON *report;
    char *str;

    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "report", report = cJSON_CreateObject());
        cJSON_AddStringToObject(report, "name",   "IoT Camera");
        cJSON_AddStringToObject(report, "type",   "device");
        cJSON_AddBoolToObject(report,   "excl",   MS_FALSE);
        cJSON_AddStringToObject(report, "desc",   "翼辉 IoT Camera");
        cJSON_AddStringToObject(report, "model",  "1");
        cJSON_AddStringToObject(report, "vendor", "ACOINFO");

    /*
     * Add extension here
     */

    str = cJSON_Print(root);
    ms_printf("REPORT DATA: %s\n", str);

    cJSON_Delete(root);

    return str;
}

/*
 * Create INVITE data
 */
static char *iot_pi_invite_data_create(void)
{
    cJSON *root;
    cJSON *report;
    char *str;

    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "report", report = cJSON_CreateObject());
        cJSON_AddStringToObject(report, "name",   "IoT Camera");
        cJSON_AddStringToObject(report, "type",   "device");
        cJSON_AddBoolToObject(report,   "excl",   MS_FALSE);
        cJSON_AddStringToObject(report, "desc",   "翼辉 IoT Camera");
        cJSON_AddStringToObject(report, "model",  "1");
        cJSON_AddStringToObject(report, "vendor", "ACOINFO");

    /*
     * Add extension here
     */

    str = cJSON_Print(root);
    ms_printf("INVITE DATA: %s\n", str);

    cJSON_Delete(root);

    return str;
}

int main(int argc, char *argv[])
{
    struct ifreq ifreq;
    int sockfd;
    struct sockaddr_in *psockaddrin = (struct sockaddr_in *)&(ifreq.ifr_addr);
    sddc_t *sddc;
    char *data;

    /*
     * Set network implement 
     */
#ifdef SDDC_CFG_NET_IMPL
    ms_net_set_impl(SDDC_CFG_NET_IMPL);
#endif

    /*
     * Create SDDC
     */
    sddc = sddc_create(SDDC_CFG_PORT);

    /*
     * Set call backs
     */
    sddc_set_on_message(sddc, iot_pi_on_message);
    sddc_set_on_message_ack(sddc, iot_pi_on_message_ack);
    sddc_set_on_message_lost(sddc, iot_pi_on_message_lost);
    sddc_set_on_invite(sddc, iot_pi_on_invite);
    sddc_set_on_invite_end(sddc, iot_pi_on_invite_end);
    sddc_set_on_update(sddc, iot_pi_on_update);
    sddc_set_on_edgeros_lost(sddc, iot_pi_on_edgeros_lost);

    /*
     * Set token
     */
#if SDDC_CFG_SECURITY_EN > 0
    sddc_set_token(sddc, "1234567890");
#endif

    /*
     * Set report data
     */
    data = iot_pi_report_data_create();
    sddc_set_report_data(sddc, data, strlen(data));

    /*
     * Set invite data
     */
    data = iot_pi_invite_data_create();
    sddc_set_invite_data(sddc, data, strlen(data));

    /*
     * Get mac address
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    ioctl(sockfd, SIOCGIFHWADDR, &ifreq);

    ms_printf("MAC addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
              (ms_uint8_t)ifreq.ifr_hwaddr.sa_data[0],
              (ms_uint8_t)ifreq.ifr_hwaddr.sa_data[1],
              (ms_uint8_t)ifreq.ifr_hwaddr.sa_data[2],
              (ms_uint8_t)ifreq.ifr_hwaddr.sa_data[3],
              (ms_uint8_t)ifreq.ifr_hwaddr.sa_data[4],
              (ms_uint8_t)ifreq.ifr_hwaddr.sa_data[5]);

    /*
     * Set uid
     */
    sddc_set_uid(sddc, (const ms_uint8_t *)ifreq.ifr_hwaddr.sa_data);

    /*
     * Get and print ip address
     */
    if (ioctl(sockfd, SIOCGIFADDR, &ifreq) == 0) {
        char ip[sizeof("255.255.255.255")];

        inet_ntoa_r(psockaddrin->sin_addr, ip, sizeof(ip));

        ms_printf("IP addr: %s\n", ip);
    } else {
        ms_printf("Failed to get IP address, Wi-Fi AP not online!\n");
    }

    close(sockfd);

    /*
     * SDDC run
     */
    while (1) {
        ms_printf("SDDC running...\n");

        sddc_run(sddc);

        ms_printf("SDDC quit!\n");
    }

    /*
     * Destroy SDDC
     */
    sddc_destroy(sddc);

    return 0;
}