#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include<unistd.h>
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXLEN 102400
/* 4 for field name "data", 1 for "=" */
#define EXTRA 12
/* 1 for added line break, 1 for trailing NUL */
#define MAXINPUT MAXLEN+EXTRA+2
#define CONFIGFILE "/var/packages/Vaultwarden/target/.env"

// 全局常量定义
const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char padding_char = '=';
 
/*编码代码
* const unsigned char * sourcedata， 源数组
* char * base64 ，码字保存
*/
int base64_encode(const unsigned char * sourcedata, char * base64)
{
    int i=0, j=0;
    unsigned char trans_index=0;    // 索引是8位，但是高两位都为0
    const int datalength = strlen((const char*)sourcedata);
    for (; i < datalength; i += 3)
    {
        // 每三个一组，进行编码
        // 要编码的数字的第一个
        trans_index = ((sourcedata[i] >> 2) & 0x3f);
        base64[j++] = base64char[(int)trans_index];
        // 第二个
        trans_index = ((sourcedata[i] << 4) & 0x30);
        if (i + 1 < datalength)
        {
            trans_index |= ((sourcedata[i + 1] >> 4) & 0x0f);
            base64[j++] = base64char[(int)trans_index];
        }
        else
        {
            base64[j++] = base64char[(int)trans_index];
 
            base64[j++] = padding_char;
 
            base64[j++] = padding_char;
 
            break;   // 超出总长度，可以直接break
        }
        // 第三个
        trans_index = ((sourcedata[i + 1] << 2) & 0x3c);
        if (i + 2 < datalength) // 有的话需要编码2个
        {
            trans_index |= ((sourcedata[i + 2] >> 6) & 0x03);
            base64[j++] = base64char[(int)trans_index];
 
            trans_index = sourcedata[i + 2] & 0x3f;
            base64[j++] = base64char[(int)trans_index];
        }
        else
        {
            base64[j++] = base64char[(int)trans_index];
 
            base64[j++] = padding_char;
 
            break;
        }
    }
 
    base64[j] = '\0'; 
 
    return 0;
}

//获取配置
int get_conf_value(char *file_path, char *key_name, char *value)
{
    FILE *fp = NULL;
        char *line = NULL, *substr = NULL;
        size_t len = 0, tlen = 0;
        ssize_t read = 0;
   
    if(file_path == NULL || key_name == NULL || value == NULL)
    {
        printf("参数无效!\n");
        return -1;
    }
        fp = fopen(file_path, "r");
        if (fp == NULL)
    {
        printf("打开配置文件出错!\n");
        return -1;
    }

        while ((read = getline(&line, &len, fp)) != -1)
    {
        substr = strstr(line, key_name);
        if(substr == NULL)
        {
            continue;
        }
        else
        {
            tlen = strlen(key_name);
            if(line[tlen] == '=')
            {
                strncpy(value, &line[tlen+1], len-tlen+1);
                tlen = strlen(value);
                *(value+tlen-1) = '\0';
                break;
            }
            else
            {
                fclose(fp);
                return -2;
            }
        }
    }
    if(substr == NULL)
    {
        printf("key: %s 不在配置文件中!\n", key_name);
        fclose(fp);
        return -1;
    }

        free(line);
    fclose(fp);
    return 0;
}

//转码
void unencode(char *src, char *last, char *dest) 
{
    for(; src != last; src++, dest++)
        if (*src == '+') 
        {
            *dest = ' ';
        }
        else if(*src == '%') 
        {
            int code;
            if (sscanf(src+1, "%2x", &code) != 1) 
            {
                code = '?';
            }
                *dest = code;
                src +=2;
        }     
            else 
            {
                *dest = *src;
            }
            *dest = '\n';
            *++dest = '\0';
}

//获取Vaultwarden管理端口，检查证书是否启用
int GetWebPort()
{
    char getport[4096] = {0};
    char checktlscon[4096] = {0};
    char port_key[] = "ROCKET_PORT";
    char tlscon_key[] = "ROCKET_TLS";
    char profilename[] = CONFIGFILE;
    int port = get_conf_value(profilename, port_key, getport);
    if(port == 0)
    {
        printf("var vaultwardenwebport = '%s';\n",getport);
    }
    get_conf_value(profilename, tlscon_key, checktlscon);
    if(strlen(checktlscon) > 0)
    {
        printf ("var url = \"https:\" + \"//\" + domain + \":\" + vaultwardenwebport;\n"); 
    }
    else
    {
        printf ("var url = prol + \"//\" + domain + \":\" + vaultwardenwebport;\n");     
    }
    return 0;
}


/**
 * 检查用户是否登录。 
 * 
 * 如果用户已登录，则将用户名输入“ user”。
 * 
 * @param user    获取用户名的缓冲区
 * @param bufsize 用户的缓冲区大小
 * 
 * @return 0: 用户未登录或错误
 *         1: 用户登录。用户名写入给定的“用户”
 */
int IsUserLogin(char *user, int bufsize)
{
    FILE *fp = NULL;
    char buf[1024];
    int login = 0;

    bzero(user, bufsize);

    fp = popen("/usr/syno/synoman/webman/modules/authenticate.cgi", "r");
    if (!fp) {
        return 0;
    }
    bzero(buf, sizeof(buf));
    fread(buf, 1024, 1, fp);


    if (strlen(buf) > 0) {
        snprintf(user, bufsize, "%s", buf);
        login = 1;
    }
    pclose(fp);

    return login;
}

//配置文件内容
int Content()
{
    char *lenstr;
    char input[MAXINPUT];
    char data[MAXINPUT];
    long len;
    FILE *fp;
    lenstr = getenv("CONTENT_LENGTH");
    if (lenstr == NULL || sscanf(lenstr,"%ld",&len) != 1 || len > MAXLEN) 
    {
        fp = fopen(CONFIGFILE, "r");
        if(fp == NULL) 
        {
            printf("抱歉，没有找到Vaultwarden配置文件。</p>");
        }
        else 
        {
            while( fgets(data, MAXLEN, fp) != NULL ) 
            {
                printf("%s", data);
            }
        }
    }
    else 
    {
        fgets(input, len+1, stdin);
        char *tmps=strstr(input,"Submit");
        char *tmpc=strtok(input,"&");
        if (tmps != NULL ) 
        {
            fp = fopen(CONFIGFILE, "w");
            unencode(tmpc+EXTRA, tmpc+len, data);
            fputs(data, fp);
            fclose(fp);
            system("sed -i 's/\r//g' /var/packages/Vaultwarden/target/.env");
            system("killall vaultwarden");
            system("cd /var/packages/Vaultwarden/target && nohup ./vaultwarden > ./logs/vaultwarden.log 2>&1 &");
            fp = fopen(CONFIGFILE, "r");
            while( fgets(data, MAXLEN, fp) != NULL ) 
            {
                printf("%s", data);
            }
            fclose(fp);
            //return 0;
        }
        else
        {
            fp = fopen(CONFIGFILE, "r");
            while( fgets(data, MAXLEN, fp) != NULL ) 
            {
                printf("%s", data);
            }
            fclose(fp);
        }
    }
}

//Vaultwarden全局配置页面
int WebPage() 
{
    printf ("<!DOCTYPE html>\n");
    printf ("<html lang=\"en\">\n");
    printf ("<head>\n");
    printf ("<meta charset=\"UTF-8\">\n");
    printf ("<title>Vaultwarden服务器</title>\n");
    printf ("</head>\n");
    printf ("<body>\n");
    printf ("<style>\n");
    printf ("textarea { border: 1px solid #ccc; padding: 7px 0px; border-radius: 5px; padding-left: 5px;margin: 0px; width: 94%; height: 50vh; -webkit-box-shadow: inset 0 1px 1px rgba(0, 0, 0, .075); box-shadow: inset 0 1px 1px rgba(0, 0, 0, .075); -webkit-transition: border-color ease-in-out .15s, -webkit-box-shadow ease-in-out .15s; -o-transition: border-color ease-in-out .15s, box-shadow ease-in-out .15s; transition: border-color ease-in-out .15s, box-shadow ease-in-out .15s }\n");
    printf ("textarea:focus { border-color: #66afe9; outline: 0; -webkit-box-shadow: inset 0 1px 1px rgba(0, 0, 0, .075), 0 0 8px rgba(102, 175, 233, .6); box-shadow: inset 0 1px 1px rgba(0, 0, 0, .075), 0 0 8px rgba(102, 175, 233, .6)}\n");
    printf (".button { width: 80px; height: 30px; border-width: 0px; border-radius: 3px; background: #1E90FF; cursor: pointer; outline: none; font-family: Microsoft YaHei; color: white; font-size: 17px; }\n");
    printf (".button:hover { background: #5599FF; }\n");
    printf ("#a { width: 100%; height: 100%; position: fixed; top: 0px; left: 0px; right: 0px; bottom: 0px; margin: auto; }\n");
    printf (".center { text-align: center; height: 20vh; }\n");
    printf ("img { height: 20vh;text-align: center; }\n");
    //printf (".des { color: gray; margin-bottom: 30px; }\n");
    printf (".des { color: gray; height: 10vh; }\n");
    printf ("a { text-decoration: none; }\n");
    printf ("</style>\n");
    printf ("<div id=\"a\" class=\"center\">\n");
    printf ("<div class=\"center\">\n");
    printf ("<img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAABEYAAAEACAYAAAC+gXPDAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAA3VpVFh0WE1MOmNvbS5hZG9iZS54bXAAAAAAADw/eHBhY2tldCBiZWdpbj0i77u/IiBpZD0iVzVNME1wQ2VoaUh6cmVTek5UY3prYzlkIj8+IDx4OnhtcG1ldGEgeG1sbnM6eD0iYWRvYmU6bnM6bWV0YS8iIHg6eG1wdGs9IkFkb2JlIFhNUCBDb3JlIDYuMC1jMDA2IDc5LjE2NDc1MywgMjAyMS8wMi8xNS0xMTo1MjoxMyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0UmVmPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VSZWYjIiB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iIHhtcE1NOk9yaWdpbmFsRG9jdW1lbnRJRD0ieG1wLmRpZDoxZjJiOTdlNy0zYjFkLTQ5YzItYjBmZi03N2M5YmUxOTZmNGEiIHhtcE1NOkRvY3VtZW50SUQ9InhtcC5kaWQ6MUQzN0MzN0EwQjkxMTFFQzkzMTA5M0MyQTk4RTgwMDUiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MUQzN0MzNzkwQjkxMTFFQzkzMTA5M0MyQTk4RTgwMDUiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIDIyLjMgKE1hY2ludG9zaCkiPiA8eG1wTU06RGVyaXZlZEZyb20gc3RSZWY6aW5zdGFuY2VJRD0ieG1wLmlpZDoxZjJiOTdlNy0zYjFkLTQ5YzItYjBmZi03N2M5YmUxOTZmNGEiIHN0UmVmOmRvY3VtZW50SUQ9InhtcC5kaWQ6MWYyYjk3ZTctM2IxZC00OWMyLWIwZmYtNzdjOWJlMTk2ZjRhIi8+IDwvcmRmOkRlc2NyaXB0aW9uPiA8L3JkZjpSREY+IDwveDp4bXBtZXRhPiA8P3hwYWNrZXQgZW5kPSJyIj8+HMemrAAAcNhJREFUeNrsnQXYHcXVx0+UkIRAgoYggeAanKDBJbhLcWiLt9AiLW3xCgUKbXF3LV4cgrtr0OAa4iEh9s2/c94vl5crKzO7M/f+f89znsi9d3d2dvTMERHSisxs5HdGnjbysZELjSzDaiGEEEIIIYQQQkiz09XI+Uamt5O3jAxk9RBCCCGEEEIIaSU6sAqCpJORdY0sa6SbkaFGnnF07cONnGGkY5XPnjKyoZEJDu4zj5G1jcxv5Gsj9xj5lq+WEEIIIYQQQgghjfij/NiaY4yRIxxct69Y15npdeRoB/cZZOSxdtd9wMhcfLWEEEIIIYQQQgipx1ZGpkl1pcWxOa99hdRXikA+N7JIjnvAHefrGte+zkh3vmJCCCGEEEIIIYRUA24zd0t9xcUvM157IyPfS2PFCORqse48aVlCbKySetdem6+ZEEIIIYQQQggh1YALCuJ71FMsTDbys5TXnd3I85JMKQKZYmSzlPfoZ+TFBNc+WbIpXQghhBBCCCGEENLktI8tUksmGtkmxXWPkuRKkTZ5U5LHBOkjNoZIkusixkk/vmpCCCGEEEIIIYRUgsCoL0lyxcVoI0MSXHegfnd6BjkuwfVnMfKflNfdjq+bEEIIIYQQQgiJmx5i3U1+o392zXk9KDnSKi6+Exs7pBYIdHq1ZFOKQODWs1Kd68Ml5ooM171W8gdhXcrIIUb2EJsamBBCCCGEEEIIIQUxp1gribFiY37gz+eMbGikY4brdTFygWRTXsA1ZVCN6+5iZKpkV4xM1+fsUeXanY2cl/Ga44wslrHu4YYDZQyUQpPEBpT9wMgObJaEEEIIIYQQQoh/YCVxao0N/3gjfzIya8prLmrkS8muvBhmZMV21+xv5HXJpxRpkz3bXRtKkb/nvOZhKeuog5FNjbxR43pfGFmIzZMQQgghhBBCCPHLACOfNdj0I+XuMimuua/kV148a2R9sdYjiOFxpbhRikA+NHKAWLedVYyc5uCaT0lyd5reYhVOkxtcc182T0IIIYQQQgghxC+DEm78Pzeye4LrwU3lYYdKjFhkqtR2AapkOSN3JrzmOZI/1gshhBBCCCGEEELqsHNKBQA26/PWud5AaT2lSJuc16Cu4cLzRYrrPSQ2Sw4hhBBCCCGEEEI8gDgXx2dQADwjNjBrNU5sYcXIxzXqZG6xCqW014OVTi82U0IIIYQQQgghxA950t8i9e1xYpUrANlrEEz0oxZWjEwzcrKR+SvqeD0jT0t295xF2UwJIYQQQgghhCSlA6sgFXOJjXexSo5r3CzWPWQpI8sbmZ3VKq+JtaoZY2RvI30yXgfKka2N3MEqJYQQQgghhBBC3IN0sOOldS08YpAj2EwJIYQQQgghhCSlI6sgFbDu6M5qCBpY4tASihBCCCGEEEJIIjq3yHNio9yWxnWK2FgUWVicTSZ4FhOr8Mv6jrsY6aTtZAqrkxBCCCGEEEKam1awGJlDbOaXN4y8auRCI+sY6ZnyOtgsUzESPguLVW6kAd/vb+RYsSl/3xEbC2ZNVichhBBCCCGEkJiZRTe41bKX3Gtke7GpYZMws5FrpDnicHxl5F0jT4rNAPOhkdFN8mx4jqUSvlO4RQ00cqaR76pcC/+HNMt0zSGEEEIIIYQQEiW7JthIv2DkF0YGJFCMvBS50mCEkbOMrCw/dqPqbWQ7I3c1gWIEaZGHNHiXsxpZ38hlYt1l6l3vQbEKNkIIIYQQQgghJCrgJvTXFBtqWE0cbWSlGteDZUnMGWneNLJxgno73sjYiJ8Tio4/1HmHuxi5NaUyaW52J0IIIYQQQgghsYG4EWdLNleMPxvZoOJasC64LmJlwTdGFkxRd7+RuK1GPjBysMyw9ECa5V8aeSzj9eZjdyKEEEIIIYQQEhtwFTlT8lkeXGTkPCOTJW7Xkn1S1t1MRoZK/G41iCNzitigu1mvMdLIPOxOhBBCCCGEEEJiA640p0lzBBTNI49JtuxDm0j1gKStJh8ZmZPdiRBCCCGEEEKaV3nQrEwTGx+i1Xle6yIt96pSoNWBSxaz0hBCCCGEEEJIk9KxyZ/vhxZ/v5OMvJzj91+yi/wvEO1UVgMhhBBCCCGENCcd+XxNDWKjjMzx+2/YRWSMZLO4IYQQQgghhBASAc2uOPi2xd9vdyOL5/g9s7HYOCOEEEIIIYQQQpoUutI0//tdMuNvkYmlH7vI/9oQLUYIIYQQQgghpIk3zs0MT/tFVjMyf4bfbSq0GAFwJ5rCaiCEEEIIIYSQ5qTZFSOj+IplKSN/TPmbvkaOFeuKwz5CCCGEEEIIIaRp6dzkzzehiZ7lAxWk3/3CyI5G1kr4292MvGDkvATfncvIFUYWS3htxHE51cgsRlY1spCR/tI8SpWJQssjQgghhBBCCGlaQlaMYIO9udgTeygChhl5VzeqSRkd8bv5zMhbKq8YedrIGxWfQ3lxkZHtE1wLSopzxSo7/ia10/AOMXK0kbUTlvE7sQqaoRX/t4CRgSpLqSAAbNdI38NwSa9gQx0sYWRhI1203d7D4YYQQgghhBBCwqNDoOVax8i/jCxb8X8fqZLgbSNvGnlVFQXj6lznECP/jORdjDXyrJEXxSqBXtFn/KHB+7vAyP4p7oN73G3kPZU5dAMPBcYvUlzncyO7GHmswff6G1laFQV4n5uIDewaC0ONbKXvpxZQ/Cyjz4hnXVL/3aZ4hMXJX4ycZOR7DjuEEEIIIYQQQuqxqG66pzeQz3VTfplYK4f1jPSuuM4xYi0apgcsw8VafkCBs6aRWTPUVycj52S8/xe6UU/7u4/FWoRkYUUjBxu5M/B3Uyl3GVmw4hmg4NnLyD+M3CFWkTU1wXV+ye5NCCGEEEIIIaQRh2TcvCJ7CKwt/mPktoA32Yj1AeuBrcVaFrhwZ8I1Timo/LDcWd5BmXupMug4sZYxoStHUMbLjDxq5MOEipD2cq/YWCyEEEIIIYQQQkhNjpJ4LAmSCCwyYHFwkNjgpP081t2Jnp/lZVXmuAbphDcy8m8jI5rs/VfKI2KD2xJCCCGEEEIIITXZu4mUIXiWAUZmK6juYDlygpFpHp4J1jiLey4/ArT213p7twkVI9cYmZldnBBCCCGEEEJIPQaJdYuJbdM7xsgTRg5XBULPkuoPWVCONzLF4bM9KX4sRerRR2wwWATb/aFJFCNHsnsTQgghhBBCCEmyIX4sos0u0upeaWQlVUqEgEvlyENi08+WBaxt4Ib0vNhUzbEqRWDFsxm7NyGEEEIIIYSQJFwQwSb3HSN/F78xQ/LQ0cifJJ9bzcMBPR/chGBB8oyRyREqRoaLTedLCCGEEEIIIYQ05FeBb3JhibFTJHV5fMZnRBraOQJ9JmQueiEyxQgyJXVn1yaEEEIIIYQQkoT1xcbsCHmje35E9Xlyyme71UiPwJ9pq8gUIyewWxNCCCGEEEIISUovI88FvtF9X8J1o6nGCQmf61IJ37IBbkLnRqYY2YHdmhBCCCGEEEJIGq6PYLN7YGR1uo3YGB3VnuU9I/tG8hyLSVyZi74wsjy7NCGEEEIIIYSER+eAy/aKhB/HYyMjF4kNBhoDcJF51shaRhY1MkBsVp1PjDwqNjVuDKwu4cY/qcZLEdUtIYQQQgghhJBAQGrT0OOMjDOyasR13CnCMiP2yXUSlxvNGezOhBBCCCGEEBImHQMu2/NiU5yGvknfMOL3PzXCMsPKZZvIyvwahxpCCCGEEEIICZOQFSOIIfFxBHU4xMhcbEqFsYaRmSIq75dGXuVrI4QQQgghhJAwCVkxEoubxyAjA9mUCmFWI9tFVubJEqfLEiGEEEIIIYQQR3QQG+QVkkYR8zsj0ySOGBKnS9hKpmYBCqgpEld8EcjdRmZO8ZzoK50k7ODIhBBCCCGEENI0Sgtf9BKbtWUFI910g4jT8++M/KD3/lb/js9GGpmov13GyGlGZomkHj8wsoGEHxMl9rZ6pLaLGLnYyKUV/55brPID0lusexBivvTRvoO/Q9mGNMpPic3SRAghhBBCCCEkErC5u0XiO9nPI9vztXsFSrIXWqxNtclosVmaCCGEEEIIIYREAE65T27BzeuNRrry9XtjpRZVirQJAhH3ZDMghBBCCCGEEPdKDNdAObBTC9YlstMszCblBbjR7NDidYDMR7QaIYQQQgghhBDH+AoY2opBIxFccwM2KS+0qrKtkqlCiyRCCCGEEEIIcY4PxQgCrD7RovW5p5EebFbOWcvIQi1eB+ONPMqmQAghhBBCCCFu8aEYwcn2JS1an6saWY7Nyjm7it8MSjHwiJFP2BQIIYQQQgghxC2+XGmeNvJQi9bpth7rtRWZTeii9L2Rs9gUCCGEEEIIIcQ9vjbw2Mid36J1iiChs7BpOWMTI31bvA4eN/IUmwIhhBBCCCGEuMenZQNM/19twTqdX2jh4JJtjMzU4nUAJeNUNgVCCCGEEEIIcY9PxchXRi5swTpFRp5d2LScMIeRQS1eBy8ZuY9NgRBCCCGEEEL84DsWxu1GPm/Bel1b6P7hArglzd3idXCOkbFsCoQQQgghhBDiB9+KkY+NXNCC9Tq72EwqJB9bGenWws8PpeIdbAaEEEIIIYQQ4o8iUqAuJTZ4ZO8Wq1s889psYpnpr3XYr4Xr4Cgjfzcync2BEEIIIYQUxHJGltW/I9bfSCMPGxnFqiEkO1C+/FM3d60kIysGFJKeY1qwzVTKt0YWZjMghBBCCCEFsblYN+6P2q1LxxlZgdVDSH7W1g7VapvbM/nqM3NviytGTpNiLLoIIYQQQkjrMpeRI3XtPaHGuvQLI8uzqgjJDzK1XNmCm9sXjczM15+alcRaTLSqUgQZnaiVJ4QQQgghvljdyHlGXkmwNv1ErHsNIU2tsCiCKWItRlqNJYxsJDY7D0nOlmID2LYqPbTPEEIIIYQQ4gokNdhDbOZHuPwziyYhBTNYbMrRVjz9v4yvPxXdjTwore1GAzlD6EpDCCGEEELygfUkkmH81chbRr7PsC6lxQhpenxZjCANcBcjXcVqJn9hpGeL1vFqRhYUG8SINGaQkVVZDXKgkVvEmjdOVqEVCSFugXXWxmL9q8vsX5iLRxu538gIvhZCCCEO6GVkHSP7ij2knk146EZI3cVYXhBDo7tKHyOLiXUhGWBkaSOLGpm1hesYdbGpkfPZ3BKBgbsnq+F/CsUHjHxoZJiRd/TPt418JjY41vf6JxUmhGQDc9YfJIyAcp+LdSOkYoQQQkie9ePcRrYSe8i2qBQXOoGQqMnSURYy0tvIAjJD8QGBidZsrNKq4ETyQiPTWBV1mdPIJqyG/wcWV4urVIJ29K4KFCVQmLxn5BsjH4t1WyOENKaTWOVIx0DKQgghhGRhbl0vIn7IbmIPrAkhnoCp8alGhgvjP6SVL42szCbUkCERvdNvVDExLaAyTTJyo5EN2ZQISTyv3SzWd/q7kvsvLMEG8pUQQghJASzT9zHyuOc5ijFGCFGgdbw90A0q/LLHR7CRPpbNqCGnRaQYGSo2FsozAZZtojBOCyFJQQYspAiHtdpRRq4Wq/SkYoQQQkiozGvkL6qwKGKOomKEEGWTgDeo1xl5OoKNNDS5s7Ap1WQ+Ix9EpBg5S8u9nVhLjdDKdyebFCGZWVJsZqgi+zYVI4QQQpKyaMHrSipGSNOT1K8aHWFqoM+AzB33RVDXqxtZg02uJnA1WiiSso4zcof+/T9GrgywjAuxSRGSGaQzPEJsJP9JrA5CCCGBAcvGGyr+/YWRu4z8y8jRKmeKPXQkhCSgc4rOh1gKoQWHm64d/l1dxPYIuK5RdxsYuZfN7icgyOjGEZX3eSOPVvz7+wDL+D6bFSG5gVvNsrrAJI3Z0cgiYpXHd4sNCk0IIcQPUIJ8beRZI6/pfmh8u+8g+cNvxcYhIYQ4YCax2S9CcxcYrosw5OR+UcJ3v4ASpz+b009AaudvJR43mt9XlH1RCc8FCDFGtmezIsQJyMBWRGDW2F1p1pYZ8b5+MLItmw4hhASzj7tH6EpDSF2SutLAlBjBQ6cEVv7JRrpoh30wgvqGewODYv4UuNHMHklZPzdya8W/fybhua3crkIIyQ9O455kNdSlm9isdW3pITuwSgghJBiwjzta902EkBp0TPFdxPL4R2Dl7y/2xB4gTek3EdT5VsLc4pWgLmI6WYQbzRv6d6T63CKw8r1p5Dec/AhxuqB8nNVQlwONrFXxb1iMTGO1EEJIMMDy/zFWAyG16Zjy+3+VH8dWKJvOFYuxZ7XTh87WQneaSvoaGRLRBqnSEgMKnRUCKx9OBD5msyLEGbBIfIfVUBO4Qh7JaiCEkKCBwppxDgmpQ1rFyLe6ABoT0DMglXA//TsyhEwNvM57GlmHTe//WV/rJAbgX9mmGIEL1w4Slsn4aWIjkhNC3PI5q6AmJ1fMwYQQQsIESv7nWQ2E1KZjht+gU4V0OoRAQKvr32+WONxpEJdiFja///ml7xRReR+vaF+bGRkcUNlgyfV3nfgIIW4ZLXRPqwaCPG/BaiCEkCj4gutEQmrTMePvrjByZUDPgbgdcKvBif5TEdT7mkYWZ/P7n0tRLNYzk7Tdi7a1XSR5umvfjDBykG7eCCHuQaanr1gNP6KXkeMlHos/QghpdbCWHctqIKQ6WRUj8FNDlppQYhlsKTOCsF4WSd3vwOb3P4VW10jK+r6Rofr3lbTNhQKCrb7B5kSIN3DCNpXV8CN+Z2QZVgMhhEQD5jEeohFSg445fvuZkT2NjA/gOXqLjTUC7pY4gk/ChaRXi7e/WNxosCm6SWaYH24j4ZySXiMzLFkIIf5gCtoZLCs2Ew0hhJB4gEvoGFYDIdXpmPP3jxg502F5XjdyvdjUuzgBT5PuD64Es2qnvzGCul9IfpzesNUYqIvrGIDp4VX69/5G9s9wDR/BG98ycpgwLSYhpDjgQoh4Rr1YFYQQEhWw+B/FaiDEHz3EZsKYnkOgXBmsygJk+4B7xcJG1jby7xTXaUv7umLO8hQl17ZwuzlNN/QxvKeHKsp9aIrf3WpkVyObilUCIZ7KAUYudlAmxDvYgMMPIYXQ38hHnscZWGEOjKAufpvgWWBJujWbDSGEBEUfXdNmmaMQx3E5ViEhjTkjYycbLo1jbcCqBbEo3k1wvf9UKFaeimDD/bWReVuwvcAk/Y1IlCKQvbXccxp5O8H34cq1j9Q+UUUb3VZs3JKsZYK1CFNkEkLFSJFsbmQCFSOEEBIls4u19qdihBBPYJOXRfs4TtJlJEmyIJtc0WkPjmDDDYuJg1qwzWys7z8GpQjS886j5d4lwfeRCm1QwnpYWmwK4CzlmqTKFUIIFSNFsLKRbxM+CxUjhBASHjiwu5WKEUKqkzfGSAdVbmTpKPBRfjTF9/9r5KwG34Hv89769/vEplgMGdTfTi3Y7rYT64IVA7cb+dJId2kcWwTRvv8kyVNGw2rmOCNTMpQLVlHrSjxZfQgh8QLrtH+IPW0khBASJziQncBqIMQt0DgupZu6LO4Aw4zMl+G+CxoZIY1P7GczMpOR6yR8iwScwK3QQm0HCpE3JR43mrZsR2sn+C5i7cycsj6gnDw9Y9kmaBvfUKybT0cOTYR4ob+0rsUIlMJXpHwWWowQQkh4IKPiNUKLEUJqbsqSAmsMBEfdXWzWl1eNnCQ2SGpaXlAFR1pGGnm2wXfmFhvgEq4GN0fwDhAIaZcWanNbGlkgkrK+qW0VJElNea+R71PeA9r7/2QsH5QwOxu538hjRv4oNvBwbw5thBBH/NrIHqwGQgghhDQzSRQjfcWeSONU+wmxaUsRI6JTjvu+mGEDCRCXopGbQgddxKF8T4sNhBkyKC+sErq0SJuDYiQWNxpYY8CiZ4CRLRJ8/4WM9/lObCyTPCwu1o0HZbjayL5iY5gQQkhW4Op5IquBEEIIIc1Oxzqb9bWMHCnW6gIn0oeJVZK44LuMv5uW8LdLGNlRrNnXTRG8B1jdbNoC7Q2WDCtHUlbE/bhX/44UvbM0+D7MDD/M0R/edlj2zcSmBEbkccQFQBprZrAhhKQBLp7nCF30CCGEENICVFvw4GQc/sQPiw2QOsjDfeGj1jnD7xBocokE34P1xcH6dwRtnRb4e8Cme8cWaG9IzbxQJGVFpiUER4Wr084Jvg9l4lwZ79Utx2/rgUCJhxu5U6y7zjFGZuWwRwhpAMa9S4TBVgkhhBDSIlQqRhCs9Cgjdxj5mWRTXCRlTUkfpLKtvEkDlS6uizu40zwbwbtYQ2wAzWZmI4nHZQiWUggguI0kV1oskvFec2p79cmqRv5s5BaxcXgIIaQacHW8VMJOHUwIIYQQ4pRKxcgqRv5a0H2Xl2yRjVcTG1wyCYikv6Fubm+L4F0g484OTdzWFhU/1kc+QFyRx/Xva0pyU/LNM95vyQKfbT2xClCm+SWEVAPuM1uxGgghhBDSSlRu+FYt8L6wGoCbTq8Uv4FFy9/Euh0kAadey+rf4RYxIvB3AQudTZq4rSHOxXyRlBVWU+/o39OcmiK70Dop74XYH8cX/HzrSTwBcAkhxXG2kT1ZDYQQQghpNSoVI0VvlFZPsSGEIgVKkTTKm8lGvta/IwvOIxG8D7jTrNiE7Qzvb3BE5UXQ1Sn69+kpfgf3sL+m6EtQhkFBuHDBz4f7duDwRwhRMB6cJTbQNCGEEEJIy1GpGBlq5IeC749MN4jlsFSd7+Cz2zMs2JDa93X9Oza590XwPhBrYkgTtjO4Tq0ZSVlfM/Jkxb/TZouBwu8uaWxpsoCR88RamRTN3UbGcPgjhIhVXJ+l8zEhhBBCSMuDE6PfG5kq9pS8SPncyL+N7CM2nSusJn4pNgDcZxmv+bxY95s2BhgZVsKzpRVYtvRqsrZ1dAT13iantSv74Rn7xKdGjhWr2JtNrBUJMsIgK8+BYl11yni+V1QpQwhJTn8jH3num5jrig54ilhc//TwLIjttTWbDSGEBEVPI9dkHNc/kWzxIQmJmhPEWo6UsWmDZccklTwKmolGtq/ybBdFsDFH3W/WRO0JSp4HIlGKYDG/Ybvy9zbyasbrTdO2+IkqvN7Vf08t6fmeM7IMhzhCqBgRm23rWo9jKRUjhBBCxQgh0XNEicqRvAIXmr/VeK6tVekSm9VCzKyjyoAY2g4UON1qtJvvIu0PbYIsOwtxaCOEihGxgbDvFL9KZipGCCGEihFCoqFzjf8/w8j3Rk4XG1AyFt4wcrKR62p8/rDY0/+VA38OLChh3vxxE7SxteXHLk0h84gqcdqDdM/7adtaKsJ3gJgne0n4mZkATPthZQT3I7gedakYq9rGIrwjBFeG8rbNumy0kS9kRtBcQkh1lhAb32hdVgUhhBBCyIzNRi3O1Y3U5ZI8RW5ZIFgmMomcb+SrOt9DwMmhEr5iZFEjgyR+xcgcRnaIpKyIc3N9nc9vMfKM2Lghm+g7igEoCX8hYQZbhSk/Tq4XUFlI/91X/42/18ueM02fCwoSxHRB4Nw3jQwXe/r+mjDILCGVwILvQiOLFXCvH1jdhBBCCImFzg0+v0HsiexlYk9wQ+MOI/eIVd6MT/gbmJDta6RP4O9miG7GY15cLivFBxPMCuJvvNPgO1CeIDvS4kZ2EhsLZlDAz3SBWKVIKMyk9bWSkSXFnlxjgzZnxut1rBiXoGSpTHU9wchjYq3EoNAaGti7QdmnlXRvKJum5/g95o0pkZa9lcGYdX6Bc3nXJugrhNQDMcj6i7VunEXnOPwJ68bu7b47SsdNWDx+KfYQD/JdC9TT/LpeGqD9GeP400ZucnDt5XUd0UfrHsDiHQeLOBwZHlld4VCon9ZZXx1HZ6loT3i2MdqeRoq1lh0mzWFhXiQLGplX6xh/dtP+PJOuMaZqHWNv+a3WM/ouLJPfasL66Kl9FOvzTtpH4Tp1lsf7LanjZy8dQ9vmetT1WO27H2j9B6MYAXAj2NnIlbr5KJvJqty4SwfVtIvkl3SwDN2MGO40fxF7Ah4j6FibRFJWKJ/+m+L7mIROEmtVta2+q9DSLCNOzTGBlGVbVYjAN3WNisWLT7pr+9tEB1ik60YMmfMCqI9zddFT5mYPk/3x2paTAremg41spIuGMhUjp+rCmiTjWK2zosDiElnukI68hwPFyDdiXRnf56skJYLDnkXEutRik7+obrBm04V+h4TXGVuhHPlUN7XIGPeskfeaqL4GG9nCyPpGVqiyt8iqGMEmaj9dV6wqtTPtYa3/mO4Zngi0jrAZX0efY2ndKEIxMkfC30/UefxFIy+LPQh6hl31J6BOV9N16DLad6EUSXowN7VCMfKS1vfz2q6+j7heoJzYXvvoeu0++8qxYgTKyy31PSyh956nzve/UsXIWzJDkTqyiEVmUtbWwWW+kl4eKuhqI7eLjQWRhyON/FlmxC8IFWxCzom0s2HAeUoXDaEDS5FVJLvbRQ8dVOA29DNdyJcJNrwnlFwGTO67GdnAyJoSTpyZe3Sgv6fEMozShV3ZQDn8aIrv40TlBp3YygZt69qC2/Mj4jfVNSzShuji1hVQEP7LyD6RL2qn6jjyjI63y+pmdFKJZUI5Htf+nAQsABfTdUdIFjA4rUPstQ8dXKujbjgWlurxusTRPTropmRUAfUzl87rG+hCHhusrp7u9bZuAl7QsfbdCPsq5om9xZ4+o8/OXuN7yBJ5QIbrw2p3d91cJeVrsRa0J4o9XA0BHKhtI1bJtpy4C1nwhbYf7JeqxVvsqXWxa4Zrf6pz1KsRtcftxCrnVtLNuMu+i/HnDe2zUPTdGVm9YC23llhlbzVe1HrLy1La3jbRvVZWoDgeKvbg99tQKnI1XbQVmU3jLVVkLO14ovtcws8kMlTCdGFKwuYST8aWix09cxftI22dtox010eW/N5XN3KJ2BOMUN83TJf/LeVZwH0aQB2M0neVdsF7fSDvcJsSFH2xZaVBLK2nJe5sWm2CPtPmKofF7XPaj78uUdCH1kjxPmA987HODV8HJDgQONrhHPh3sad6vsr7jdbh6p77/GCxFoavldTmoRS5TMsRA+ifZ6jCKsnzXZjy+ti83ZWzTvH7OUusoy6qDLpP+53P9gMrBhwA7VBFMdLsWWnwjKfo/De2wDXV3WIPSUMFyvmjxFpRjUvwTC84uB8SiXzguK6hmAspTMD/ND5FNLJnVMPk64TuhggWg9joDpL4mEkXFDEsuOHLtqnj58dpFk4GDxNr+l3Us1wl6azAXG8cz9fTilg2W09LOYGYf2vkVrEm0zEpRjroIutmsSebVIyEqxjZLxLlfxbFCGIKfBVIuTZI8U4OCLh+/+Jw43dZQWX2pTBADLFbVAkTwrv5WueLxQNd68HV/nYdv9I8VxrFCOad4Y7qE3XZo4S62lHsiffEgtsPFDCIx9iWMKB7EytG2hQib5XYX6G4vU3CSvIBZc2lqmxN8ywv5JgHfpfhfmlkku4ze4ZQwYM8PihiPdypL3EOz8+xnWpUQ18Qwi+8s8RFv4AWFY0EpmKdPNZFX104PK3t2+ezwFKjaDceLIyOy7gJm6x9EJse+GnCGuFMI/8wcqMOyjgZnOaxzj6W4jMnob3Notp0nDifVUJ/yaIYEW1fKPvcuon/nYfTACpGsgOz9XMjmduyKkZgPfpOyeWZquPXOinezZ66SZnieUxLKtO0LHiOPzlUjPxbr+nzOVH/aznu4yj7kRnmsmk6t3+vBy3vqOLue5mRWt7FMyMmycElHn600VkPfk7SDei4jM+TVDGCd+L61P/4AutrCT2IHZuhXU3WDeBIPXSaoG1taoZnxruC+ywCA1/ZZIoRrEsQJ+O1gOYsKEguLmAvW6s+ZldlPLK2fpfxGbIoRlbRe04pqJ6hlJ237Aa4vcdF4fraaYsA93lPwl8Q4mR2LomLHSNZbGPiKSoWB7Sa6+lpxXhPz3N/wYoRBLB6KMMkPUpPMLZSxVFb4LpuuujqrH/HBhwxjY4Q68fpS7GEhd0vS+wvXVRJclnGBU+RipH2dNB3+FddwFExUp5iZKAqGKc3oVQqRuAjDv98xCgYJsWZSmNxiaB7pxvZXw9X0sQKwoHBxmLjVRyhp13PSXFul1P1Gd7QjdHRWhYohgc46icYD5bU97O3Kk6v0JPDvMq6Sbo5/K+O170c9u8VdC5Lo8SAkutesYGN19UNUB9tE711XISVxxBtMy+q4iTv4eHp4i/GST3wbBvpGmaUg7nqwgRt6TcOFUvtN/jLFDCv76cKrbTjzCM6RgzRcs6ubQobwLX1um3JGdJYoHyu/f7aJlKMIJD9+SnGl/GqtMCc8rKOJ/fpWPyJ9lGX67BHCmhrbcyi64BzVDmbd02WRjGCPfWvCpyPK+U2KTGOITrlJZ4e7DUpPp7GGZEsCjePSCnSWeJwU2qbgJYpuH466MLGx/O8r6dovk+UYPGwT4YF/be6IF81Qxm76aD7nae6wwRyUAD95/LIFCOVnETFSGmKkc4e5+bQFCPtNx8b6+b7S4/3v1o3/K4Vz7ge3IURl8qn1RhOi48XG7S2jODgWLTuITbAa5bywwrjYEmX+SUpO6dUWEzVjdQGKcvSUTe1dzjYPPxL/Fq6Vm524IJxpB7SubQAaqQYOdzzQcFJHuutd4b9BfrG31TxkLRdYSO8r9gg0Enb8JQc7S80xci6quhNogx5W5Xp26vSspPMCOTcQf8ONyNYAcKCDlYPIxy1tWHiLxNqBz1A3CXjQaULxQiU/teVvEY4RwpOqNJVF6IvenyoF6V4X6HVPGmjfSzKYmE+j5tX1/JQSXX0J48TPrTmJ6oW3dcg/JeMm751HNx/Q7HuL75OVPcruf8sXcAG3JdiBCemRbnVUDESr0I6i2Ah3yia/fKqWHTpw4+N4C0Frkee91B3V+hGLQSgFB+eUmF9lfjLhriLpLPghHXhsY7u+2HO93qcx/cEC0bEE7xYsrvK5FGMbCn+LGsrLWx9rYHvT1GOiTp2583gCAu2Zz3XWSiKEaxD90q410Cd7Jlx07yhvhsXgXKhZNnVYR3MonPiSeIu/k4WxchyqkQKISbnz4tqgMvoAOb7oZ4soXN1LWAgcWXZMHckipGfSxj+00lk75Lq6IgCFHKvqGbc9elaFqXI57ogdgVOPb/xVG8jxX+2g0YUscH1oRjBictpVIyUohjppH3znQaCkyu4NUzw/CxT9XleS1CmRgKX18ckeWa6/bQfuwrOXWTQS2TMcOkOBeuE7oGtEfZNUf5DPZZj55T9YKz+xhXo62/n7GMbOCxPJy3T4QVtdC6ss+f4soD7Y400u+M2hdgrz6Qow7vaH1wBBajPk/sQFCNop3+WZBY4x4kby6rtxU3AefTZvJbJC6pi9aYC+kgjxQjCAxSZYCKJZWEfn40PUZt/JcVFtn+sJK3jwZFs4oNKTVSHByKpzxElKpt+LsVFJr/EoVLi1IBOtnYTfzEtnpFyota3cWKkihHRCZuKkXJijGA8g9n7InVkgCoWfafxxYYTcSD6NihPEkHwwoUknQ/x7uImJtFrJfR/bK5cWMXhpD/ErHb9E5b/zx7LAIVCmpgnU8VPHKqFc77rNxzOVciiV1S8m1qKEWxqXizo/tjousx6CYu1N1Pc/1Xt6z74vfiLzVKmYgR7trMTruGWdXzvhVIqvepZdWdVaK4rxQaYracYGaLryND2dmf4anx44LsLfpj7S+poAwo4PXMhT0SgFMEidozEoRi5tMR62qPgNofFzrE5F1B7Zbw3zNu29DRB3uGxzv4o5UX/30H8ZxXxpRhZjoqRINL1NuJ+z88CM/iNSp6PznbwHDeXVPbtHJQd1rBzSHjMn6DsN3m8PxSIaV3+7vJYHgSrz2Nlu7ujclxa8BqsmmLkooLX1K4yPkJ5myY16eu6XvYJ5siR0jyKEViknpmgjMhuOLunMuC6tzuoxy8kW8DrdaS4bC/1FCObSvI4NZ/qegNueX8X6/pzis7PcH0dKm69DNDmncaORACVf4v/tKLV5J6SOhv8zm6MYCOPheaSgStGjpXismrklcEl1tMWJSmQMABtnaG80LxnjRuDQXxPT/V4oMe6wvtZtKT2MUgnzhgVI/CvLsIaioqR7MC14uEC5qutS56PYFae93TtzhLLf3POst8o5ad2rcaG0jhQoU9rzrSpSjEXbOKxPHnjA8FKwUV8vm3lxybpiO0C10gEXj1U13cu3TTaK0Z2qrNB+o9upq53eP9bHI4zaaxcXitAKdLGLxxvOstUjCSx7j+ngDGvmyMl4sMZ+23lfhWWnxeIjVuIPvorVT64yrhaTTGySYL9y1eqS0BMlRUaPOdsqmg5Xty5yDvLNnqglJsD+poSJ+ptItjIY3A7NXDFyDMSh1LkHSk+A1IlQ6Q8yxpsWJDaLGmgL2yiHs15z4s91SMi/I/wWFfHOTxRSnv65DuIqS/FSF8pJg06FSNUjCRhF8l3wjasxLKvlLPsoQZt/3mDcu/p8d4/y1CPCIjrOwMMTOTznO5v6KAMyDqJ+CKIowIXj2rZi7rovV53rBhZUjdT1ZIyIOvUzO3u7yK+ggsX3w4plTWw3i06E6LLmGVlKUZWS7BmvlmKydQEEJ/SRSyoszLce7CRXxtZX/tsNRZ3pLxprxjZXOq7z7yquoSs6xgoUVxYs8JNrl/eFwzzpLJP+st0bcAp54cS/ob+tQI7fpYGPVLiUIyU6SbRNsiPKLkOXpJkFki/dXCv0Y4Wbu2BcucNj3WEAHBlxKGZowAltS/FyOwOFGlUjFAx4nIBe2+O50AK3T4llR0b01tylP0BCS/wKp7pXw3mJV/uP5jzX85Qj1cWUC+dJF98tr+VoLx/V9woRmaqoehA26+VXQ8n0PvnWEdhfnWRqSltsPEyst4hoPObEq9ipF8CRdwLJYzTq0j+A85p4jagc/u57yGHipG16+zxPtK27SJmzxzixjLsgDyF+HUgm9UzpDw6SHHZFPIGU9s0UMXIXySObDQo44ol19XyUmyQs1qCDWw9i4i5HSoMocFd2MPA/7jnOhpc0nj0VKSKkdlybkSpGKFixDXIKDBWss+5W5ZY9k1zvINPpbiYNUlBisn3S9o4IuZN2theWC+cWOAaKuu7fryEdwnXhjxuk+frdfausnZ8UDf0jUBg+bQZdDAWbOfg+bGhTRN24Cop72AT7g+jI1SMdNN20ijr4dIl1etfHNTpU+LvAG5zyXdg3aYYgZVTNStmJEA408Paft6MSuxKgdvfzFm191sFMmFOLPHeqMT/SvggeOYuAZYLG7kNJEx/5vbcJ9bUv0wwMU8JoC5WbKAkOlw3hC6Aad/x4vYE8wfdgPlk2RLeS9uCPFamCyHhcKsusrIqkYaUWPbnjLyV8bc4aV0+sHeBjex8NT5DsFifQU73zLBQbssiUQRQBnyX8beYp7sW/C5v0k1pVnDaDteYX7VbO8J1Cae93yS4BtoMYqOclXAPgXueLDZmSR4W0vVMl4TfxwET4kBMLanf4bDiTokPZB/cv8F3EFfkjZLKd1qOuaUNHFCt66l8d2vby9NHofQ4X9t8JVCUIC7QUfp3l2BcQZasSTmusZYkDxnwI6AYmTOQDjBXyfdHx4oh8wsCM84dWJngA7qIxMHN2tnLZHwgG99OdRapnTxsCJCN51DH1/S90ICvdRlxRjpInCBF4AjuxUlATJUZpvpZ+uFaUt5JLzbKeWKFbCzVY0WUxZZ1NvCIhfClp/v2kmwWcnj/MxdUNy/nUDTAHWWBgt8lYoK8nvG301WpsLf8WHmHtdmvUm60UI4jtW3h4GtUlf4Pyy+4aW2vm9k8YD2AVNJLJPw+DsGOEWspVSaXiT1MioWV9V3VG7/QTi4osYxY67iI/Qjlz6weypfXmgyKBWSPWaPd/8OQYDMjt+mazwdQet6Y4/f9JGM67I5SjglerYmrTL7Wlxw60NptE1iZYK41WwR1hwkzBOVXKAtVKGieq/HZmpIzeFENEFl+sONn8ElfCTeuT4hM8zhREpKVG3JsTKA8Xq+kcmNhe0eODQ0WrysF8g6woayVwvlL3dT6AqeeWTJAQDFSVOyCbyS7UrmruDdnT8K7Odo1rIyPajd3HJlxjQblB2K0wGUEChK4ONyu/R4ZKvbU+z0g+Q+lYM2SJi4ErFPuCqD/PSbZrc+KBtbxZyToe3/XvVuZwNIrr8UExsX1PZUP7jBZrd6w32x/QPoPsa5o73iuV1iAIb5THuv6pbJu0GAi80kAHWH2AMrwsJTr0pOELrrYCQX4AK4dyWALa5H3AihHm19u2Zxbp++v56lPQslworhJL9i2IPJJXynHYoQQ4o4vJLtSHCd5O5RY9g90k5eF3rohDAFYr9QybYap/2se7w1ripkjaadZwBxVhvU31g9ZFOHYeyBuQaWlMTJTXuWgTDjsRWrhrVWBcZLYQK4jHbWjX6f8DWIdjA+gbWHdeUkk4/XGCfYVw8VaApYNXOPvcHCdQ8WP1Qhio7lwB4RC8Xht/5MKqlu41T2T4/dw4Z8py+AE8zL4cT1dcuOaKYAGDrPAGGKNwCQ0FN/hLbTxxQAWXyGYEo6Uck/VoWFHqro/1PnOkh7vjwlv/UjazPxCxQghzQA2BRMy/naw2MChZQA3gDwmxXtJGO63u4o9CW7PlALWXVDIZI1vVaSFZ1YLDMxRs5XUNl24BX+jm67QDyaR0XDRFN9H+tKQXPShIBoVwVh9cILvXC9ulF15wVr+OQfXwWHkYh7KN9rRfgPuYCcUXLdwJX0sx++heE2tbGob8KFhxakCTnK/LalxwfKga8kNfKJunkMHi5xQ3Gk2lfBSAlbj7ZwdrFnAySP8bE9p8L3enssBZWwMLiqzs8kQ0hQgC9ewjL+FSfHOJZYdm6vPMv4WsRBWDWAcXafGZ6+IXzca0EGyx20qMlD6uxmVA5hLe5bwXsc4qp/fSfkxOBoBK4bdU/4GVjBfB/QMKMv9gdczYosksUK/V8JIYgBedXQdH1nEoAjLqxiBUuS0kur2xRy/xaF9ZsUIwEnKn3TDfU0JD49Bfa4AGvg9ki+Kb1GsF4BCAgqatSJZFMPP88uAylO0FQI2BEiFCPPSRnGFYHLsWzGyjsShGOkocZhgE0KSzQNZ3BhxaLN5ieVGUM5Lc/x+Ryn34AmBt+et8dkD4v8U+yPJZi0EV81PC6wnnJBmVYyUEadvnOR3Z31I3LjQ+ATWYrAW6ZbyXYZmgQ6L6VcCr+vDEoxVUCC+GVCZ4U4z2sF11kzZxpKAck3J0b9hWf7XEusW4Q+yJsyYM8v6vZqJIE4moBXdreCG1zmQDcjHEr5GFawmtU9gigILxcUiqKvvdfINiSItsxDECmm5k/qXYuPgOysKzH7njqDtoB56CyGkGbhcF7FZgNXFgJLKjTEZVhVZfbsRLG+hEut9S6l+GIBgo0XECUCclixxHqCkGF5gPXWV7K47ZcxTUGjlUYxg4/VnCd+FZnPdtKYBacLfCOw5MI68HnA9I4bLRgm+B5egbwIq90Rxk7IWezrXLpvjc/RRPNPJJdctDrTzBA1OfQhdbwC+VmZEdy4CaLv7BdLI74tgoIZWceOSy7CuxJFWFPFzQkvFPLqAeyBaNlzkEOk9TQTpiTk2D0mZLHFkL0H77imEkGYAwSKzKsmxPinTnQbpCx/J+FvE9tispPl6FSMr1PjsSbEB9nwzLOOcO7XgzS2sDLIqv8qwIMahU54YI3DtfSDwMQOm+D/P8Lt7xU38Fde8F3Bdo57nSfC9VwOrW1ijfeTgOguJewX2pBxr7UkB1C3iyHyW4/epYy810kxDW3SsboDv8fzw8EFdNJBGjtRaoZubAWhW5y/p3vBbjiWA5r0SnqLLp9kr/EgPMbJLjk3AcM/PP0rCCJzViA4SRypqQkgyYDWSNSsY5ryyLFuxSL05x+/3l3KsCnapcd8fpLg0prgXUj+mPTnFIdnbBdbVuBybmKklvNuOkl3ZBvew0yMYLxDvIm267q8keyDdIjaaIYYLQGrepLETvwis7JP0nbtgTQ/ly6pECsHdfWLO953aAiepyR6CliGi+EEeGyTKsmQgjRwv4hEJn6WlPHcaTBbzR1BHn4n/wG5Z2nofD9fFwugysRkU/i353HXu1EWaL14saSEX6+RACHHDIzk2BsgIN6jEsj+cY8OF9ULRQVgxz61b4zME27y2wLKcZeTlFN/H/Hl8wfNUHkVDbDwm+QIrFgEOsPbP8E5gofRBoM8E14rXAiwXrJsHJOyXXwVWdiheXVlZw7rOdZbW2MeUPHGe+mYZhJOCE95zxa9rzYIBvQiY+H0deGNBY0dWmKIDec6s942Bp1MuhopgDvETCA8pw46QfP54bdzmud5CiijeiOlCCGkWsJa5IeNv4ZKySYllHyf5FNY7F7xe2ERqu9FAQTWmwLJgQ4jsJ+8kbCNIRvAGu4s3RkdQRlixb5nhd8MDfr6p4vfQKytQOCcJPPqhhJdyGGtEV8oaxFnp4mGvGDN59uKdvf9A/JqH9dWFx/gAXgRiUkCrukHgDQbuNEuJu3RRSYAbzboRdCaYj90d4MZ2Pg8DH4BpqiutNSbPU3QD4ToYFBYNt3FdSAgpaRGL7DRHSbZAl9jsny35/J6zsqGR5XP8HlnJjhc3/vBJFuOb1qhjbBrLyH54n250/y42o17PirkYLiyIFYCDhd9K4+xtvtpmq9Ax8PKh/W6bsZzDIni20Fgq4fdwMLuXWOVOlwDKPU377dqOrtfTw/uZGvlY8X2RY2oWxYhP3y4oRhaWcMy8sKleL/ABHJk94JNWpGJkDbExYUIHSrx7AizXnJ4GdET4d2mFcY9uAH7vuJwnS3g+ooSQ1gEbl6GSLU7W8jrn3lBwmRFcc5+c6xHES0LMjyLSL8J1Z3CNzxC347GS3j0sRpClB27IyxpZUesUbh1varsoK+jgWKGFYkjrtN0z/A4KtrdYfalAwNWkyTeWEXto16zAoty1+/bIyOukUEVelgl2B4/lmUvKTSnXHqSR+zyCRrOTFJfDHtYD20TSmZ6Sck71GgG/ax/mzJgwXAcwPs7I3xxeD4qWKxxe7weuKQghKYELx9U5fr+pFB97CNYi6+W8BhaYOxdUdlizLlDl/7Hxv6bk948DBAQmR9wRnD7vYeRMsS6eZSlFEGNve3FvoUmysZqR/hl+h8xXw1h9qVihxljRivTxoAiYGHmdFOp23zHld08V96fHleBEZLGAXgbMTZ+LoNHg5GOpgu61uIPFWRGMz7nw9QmsfHxYjMBH8yZ9Ry45WqxpcV6/TihFEAPFZZreOTmPEkIyMFSyxwHYvOC1CuaLfRxda+kC5nCYuw+p8dk3Ury1TahgLbCV2MMHpK3FSTgVI+WDjemWGX/7sdjAwiQ5sNrqxWr4H908jAGxxxjBPF2YJV0axcg/xabu9U3/wF7IVRL+qTTe44bi3+Wngy6oYsjSAXPZhwMtWz/x5xu5nJFbjazs+Lrwyd7RyB0Zfguz6V8YOVzc+Tpi0XKOuPPrJIS0FjjZzZoVBRvatQosK7LhbO3oWgj8vY/n8g6qUz+w1Piyxdse6gfWmIh1g3hbOHiYl10yGKDYyxpfENZok1iFqViCVfD/YG+wEKvhR7TFcSmEJOb8XXUDsl9BZYLlAyxHJgTyQu7WBdSAwBvOvvqevvN4D/gn7x5JJ7pPwg04NHcBkwwWWzDLfdrhdR9Q2U2s7zhSP9YKBIh4J88beVI3H66CNu+iGwSYg7dKWkNCiHtguYbsc7/M+PuddGzzneEB67Q9HY93g3Ue8pX2Em40tVJOnt+i7Q0m8nBD3lg33XOwCwbLSjk2pxNZfamApUgaF/D3xLrJ/yDFZ+Qsav/yDZvFjyh0rd+oUSFDzEW6GSlyQBqoG6oQQDRcBKE8KPCNGAbxVcT6yPpiSckXEb8ooLG/JODyLVrAPXD6dIuRvT20iWtU+otVGM4qVpkJiyVodeFyg+Cq74i7dIzwA4cyZFOhQoQQ4ga4yiJw+XIZlQs4yHnWcxnhsrOb42si2CGsRv7iobzz61hdDQQ3fbrF2hhiVewqNmj9KuxyUTAkx29pLZKOFSSdWyJSaEOZPaGJ64Rr3BKppxiBNvtSI1sUXKZeuvl+MqB6Qj3AYqZb4O8TCqyhngZmtJXNI1rsvhNo2fpJ8ujbLha/ONHcX6zJrmuGq/gCLlt76gIepuRdhBBC3PGtkeskm2IEc+LGOt/4MvPFmAelcPcqn32h6yRk1umd8rpQYsPyzodiBBl7ap22XyatcaKOtSKseLfUtpXGTWaKNOdJeEyslGNzOobVl4q+KcevnhKHO38emJmqRGrFpBigG6ktSirXioHV0ytGXo7gfcJM01cwSiisdo6gDmAefVPA5YP7yawF3g8TDhR7e0Q0LmFReKBYV5y2OCJUihBCfICYF1kDS8OSYy6PZcOGupYbMwKY/kqyp2JEENYNHZe3h9TOWgfr21ubvC0tLDa7zQtG/iHWwjGJUuRDsVbJaE9fs0uWCsIH5LHqHc8qTEXPDHsRBmol3uhYY7K8XsoNaghtbb+A6mmK1knozKYTsa8N/SIR1AFirNwScPmg9Cs66jwmkfPEBj8NGZyyHSBWEYnF5UCpb6U1VcKNI0MIiQMoYB/K+Fu4l67gsWzYKM9e5f9xonizkU8luysP5qFdHZcXliLb1fjsTrEZO5oNKO1htYNA/Y+LVVYtJY2V+YiRgFhoyEoDK5tzxR7A8bS4XGCxnufwahSrMBXdU34fipSZWG3EFx2rbH5vV8VEmSwr7lOO5gX1EoMmGO40rs0wYVK4bSRt+r8SduCigVKO/yAmH2SWOSLAOoFVC9xlXjLyL11UNpr4sPjYT/slIYRkZarOG1k3pNuLH4s2uDPvX+MzKEWe079fKdYaIwvYkC/osMyb1qkLuCw1U/wFKKx+JlaphnkIgen7NvgNlCEfibUmWVnXVcj09kXFpq8ju2SpLJ1z4z2BVZiKtEoo9JGurDbii8oBGPEIThVrClg2nXVzFBKfG7krgneKWAyuLTtgiTIkkjYdsmUPzIwXKLlfnW5kMwkjuBMW/oghglM2BMtdIsGEB1ep+8UGPrw8x4aAEELaQGr3YRl/i82tDxfWXWooLaDIgXVCW6yOB8W6YmRhcXF36AEFdy13W8T8apagq3gnCP4ISyMopdbSub0esGR9wsivxVqN4s/XuIkO9v1mVYz8IH4zQzYjM6f8/ixCixHikUrFCOKJbBBQ2bDBDynADhYhN0bwTmEZ8DPH14Qf8lwRPDsWX88EXD6YXM8bQDmwKOtZ4v1n0wU0/M2h3EiqBB1u5DdiAx6+ov/X7EG4CCH++UBs4PIswHJgE8flgRJ7vxrjGywUHq74N6wwrs5xL6z9XJzA4qR95RqfIQj4F5G3ESiRYHEJhQjcXvon+M1H+m4w30GBcg43zsEzv+Sz2qF7bzr6ZNjj0GKEeKOy888bWNnWlex5xH0Bjf/ngb9TWAJsLW7daeCHHEPwS5yijQ24fAOlur94GRN/WRr3TfQ9wax6zRS/u1usmfbZHLYJIR5AbKqsGVNcH0bsKNUVxnD3gVVk+8wX+L+s1ger6Xor77qjVoBvWPndJfHGzkAq0d8aeUysxeUcCX7zupF/i83kh7bxALtXNMyf47fYsM/GKkzFTBnqmMH4iTcqFSPDAyvbfFJ+rJP2fKObutBBVqH1HF0LG/k1InhmLLrg5xuytn4VCcOFBadYRbugIJjzhUbukfRuWaeJPdUcJoQQ4oehkt3dY5C4jYv2c6l+KopsJ9WCi2P9dn/Ge8F6cLOc5YUrUS2XHIz5b0XYHvBMh4pVyv9NkrlLvS02BTKsGg8x8ia7VXTMnfP3s7AKU9E7w296sNqILyoVI3dJeMqRLSUskylkp/lvBO8VPnuuTrBwcjV7BM+M05z3Ai4f6nCVQMpylhQXSLi/WMUGFpf7p/ztV0b2MnKUkWkcrgkhHkF8gKzZaVzOuchwMqjGZ3DnreaKgQOBC3PcE5lk8pyUb1RHcQBFzrjI2gLq4z9iLRSTxN1DyuS/6Zr1WInfbahVgSVCt5zXYCrZ5PSUbG7dy7HqiC8qFSMjdFAPKY4G0piF5k7zqthTm9BZW9woNHAK1DmC571Mws4aBPemxUouA1Ilwm/97oLud7BYKx7EBUmr4X9F+/8VHKYJIQWBYJpZ3WWxmXZhEXiYVDcv/1DnuVo8KdnT4SLg5KY5ylxL6Y10wkMjev/9jJxv5CaxMUGScJuunY+WsA9nSLo9UVZozZCcmSSbWzcCGHdj9ZEiBgH4Rf5MNyRnGPm65PLBJC00Nw6cDMTgToPTn7zR5qGUWj6CZ8U7eSLwMiKAbVmBQpEOEKbZMJe+pID74fTwPrGpd5fN8HsspKFIepZDNCGkQIZLdncaWBbkjdWBBf/gGp/d3GBNhhTmF+W4N9YLWU67EQtl1RqfQcHwYSTvfm+xhwaYK5MouOBajYOG3SJYf5BkdHewTqPFSHKmS7bYQwizMCerj/igmnb0B91IHakTNEzZoREvy5R9JwnPZw9uG2MCf7ew8tgm5zV2jmTwQXt9P+DyIQ3tWgXfExl6jhNrko2geDCz9u3vDAsluOkgA8FGGa9xv/b5jzg8E0JKANmysgQy7abrpTzAym7WKv//mVhLhkabDMyFkzPeG8r7gRl+t6duKNuDQLb3RvC+sb68VOs3qSL/QZ3jLhGm3G0mEO8ir/t+d1ZjYiZk7D+LShwu/iRCGpmNIWDWFTrZQ0P3e7G514sEk/V8gdXbq5I90FmRID1sHl+8TcWNaaFv4AscctBVBGKbv4D7jBab/hZmvUi9fYrY08/RBdx7B7GWHoflmLBuMLKL2JM4QggpA4xDWRWzUIBnPUxYRmorlKFgSOKm8W4OZUQXSe9Og03kVjU+g2vPw4G/a1jEImPM3ik2xMhMg0OjV9hVmg4cAOfNnkQXj+RAeZrFBR77ksVYfcQHSTe92Fi9bOTPYgODQVmB3OwjxL8lCSwfhgRWbziRuSOC94sUzFtn/C1S+C0ZwTNCURdyzBcsttbzeH1kl4EZ70FiT/sONHKnWN/uIsAJC07aLtOFfVZgwgyz5O84LBNCSmRSDuXCAnUUBY2AG3M1BToUxRckvMZ4XZtlBdZ6/VN8H+6ZC9b47L9al6Gync47qyb8Pg5fEEfkWF37kubjawdtloqRdIzM+LtVJI74hyQy0loDQJP6rVgzwn3EpoX9h/hXjmACCy2gEU5D3o/gHcNyIIvPJKwO5org+f5boBIgCziRWtfTtZ8xso7YU8ZzxfrHF5mGF246OG37ec7++bxYxU5smQsIIc3JNWJjdqQFivAsLqyIT1JLofKUjvVJgZXgOxmfe4DUzohTDVhOVHMdgMVNyG40sEyEAqlvwu9DKQL3cmSemczu0bRMcPB+0ab6sSoTk1URBS+GmVl9xDV53CQweMCSBAEWfUfihmZwxcDqDiarMbjTDJT08R5gUrt2BM82OYJ3ANPq3p6ufYMqFb4v+JkQmA6WKfc56JefGDlcwksVTghpXZ4TG4w+65yb1noOypRqFpowNT8v5bUwlt6S49nhOp1E0Q3rllpKlMdy1J9voMxBkNo0J/vHi42fRZqfvOsp9Au6eSQnq8XI6lKMizppMVzEj0DE8ac9lxPmUocGWH/YGE4J/B0jkFtav+HBEkc2mkdUMRAqc4sNTOeDL6Uc/22ciJ5q5BzJln++EpzCQSnyJIdiQkhgZE0VjphoaaxG5qgzT7wk2dKr4zdZT2I3ERvcsBE7SfX4bziwuC3Qd7qjkYslnYXjZUZOZndoGb7N+XschC3OakxM1nEK1iK7svqIa1wF1hxZQFkRZyQ0LeyjRl6M4D0jJkyagHBYGM0awXPBWmR0wOWDQmqgp2vDzPvzEp4JLjvHOLoWzNVvEVKNH1gFhJTK7ZI9lgQCbs+U8LtbSO2DiDMz3v95yWdNuX2Dz+Geu7lU9/FHcPq7A3yfML1H9pk0SpGPHc53JA7GOrgGLUaSkyeu3N5G+jRJPXRgUwgDF4oRpDfbroCyQjv468DqD4umhyJ4z0tLcqsRpK5bL4JnGhHo4qsNLL4O93h9pAA+uuBnOsPIvo6uBYuX33MIrskkVgHhIqxUvjJyV8bfwsUwSWwpxOf4eY3PkBXwzoz3H59zftyrwYZjTSMrV/l/xKF7QLJlmvBJL7FBwtPGwjpF2wFprX6fF1hcdWJVJmJijt/CYm2rJqmHk8TG2iMlk1cxApOxq6QYPy8s5JBhJbTc1TjxHhXBu94w4ftGPJJlIngeLL5eC7h8sHBawfM9DitwUkAkfpeKSZzcfcIhmBtXUgrTWAWJuDjj77AB3yzB95Dlr1acDiii88Q7QOa8rGmHsaarp9jZVpUN7cGBxSUBvkcoOFZK+RvEkbuIXaDlcKHU6y/V3cyaYd5wPXd8lnOcO7AJ6hVKZhwU/lts3M552Q3LI69i5I9GliuwvPME2AkQpO3ZCN41NtBJ0u8OFhtHIvTBOeSgq7BuOqqA+3TSgdT3BPwLsXFFXC48LhZCSBlMFaYbTQpSwb+Z8bc4ZJi7zuewFjmgxmdfGLk1Z9mheL4rx+9rnV4iJsr6NT6DC887gb1DWDVnsXS8VKhAbEXg2pE3diAsRhZqwrrB2nYmx9dEOIJhOX6PA8g1I6/XvSv+frCR2dgNyyOPYmRbfYFFglNURBSfJaA6hOnofRFMoOhojVxkFhDrGx06wxwsGn2CReNKBd0LSpF/ib+0ZchOdJrja8LE+0MOv4SUNmdNYDUkAkrcyzP+dqkG8ynmiC1qfIZMNN86KD/S2WdNg47NRrXDFLjlVrMqhUn8DQG+QxymdU/5G6znbmbzb0lecDA+QnmwShPWDazEeji+5teSTzGCLJq/irhO+8uPQx28L9aKhpREVsUI4hucqw2yaLDY2DGwerxe4kg3imBpszZYCMUQTRvZaEI98UQKwN8WfE+4mB3g4bqwSIGliGtF5P3CkzhCyoT+78mB1UWWE2Qc5Gwk1d3iME/sWWMNNsqhggFj7XMZfwtl+8+q/P8mNcr9qZFrA3t3OOzZIMPvYCX0BZt+S/KEuEkoAaXn7E1WNx3FXdKOSl7J+fstjewQaZ2i3AMq/o0sjRPZDctt5GnBxhr+mnOXWOa9PXXOrGBB8EwE7xuLtKVrfNZZPw8dLBpvC7h8WDSuW8J9/2xkLcfXPNzDNcHzQgghcQDrtqxBUDEfLFvl/2FqXytFL9IEv+2o7MhudW/ORXultQWy52xY47t3Bbig30aSpR5uz7PCzGCtCtrwqw6ug4PGVZusbqar+FgT5lFGwULnRCMLRlafiOW0W7v/w16SwfdLJItyAfEGtiu53GsY2T2wuoRrR+jmyZ11oVaNRSQONxr4L4caX6TMzElYvJ5uZC5H14Pl0CEeyjlG8qVnaxW+ZRUQEgSY16/J+FuMx+u0+z9Y6yDmRbVYXoj/4trq4j9GPsj4W1hcbF3xb1hfzFPle3DXuTqw9waLZgRBzxLIGtYiU9j0W5YntC/moVOO9tdqwGLk05zXgNvfyRLWoXkjkP2rfZKG59gcyiVtA0KMihMCKDcGHAQG6xFQXd4u2SPAFwmUWtUiHiMqcr/Ay94Wz2VqoOXDYneNEu+P04njHV0LPps+godhwTmZQ29Dvi+oPxFCGoPFata4SIjHVhlMD/NsrYMdKEVedlx2ZFfJatEKl5+2E01YC29Z43so84uBvbOFcsxhUPTE5u7J8dwdUIy4UIzBnWaRJqoXX0oexBlxkaUQ4+r+kdQlFCLt43Qitsin7H7lkkYxAteZ03WiDIHVJVukcV9M1E176CBoWnvTXlg6bB1B2b8RG88lRKBs+o2UE3enkn0lvzUVTjkHc/FWKkVY1fAki3O/LzAOdm+i+sKhR9ZgnAjGvXS7MXrOKt+Dwv8q8eOOcp1kV7auKFYpsnSdeeEKCe/AAvNY1hgPUyNsoz2EuAKuNC7i2MG1Y1AT1UsXj3vAlxytaZARcokI6hKu6u2t724XWlRHtTg6SX5q8lM2cOuZO6DywNw2hmj/O7fbwONUZcMIyv2GkdcDLRtM+PoHUA74WiLeSB7rH5h/DvBUPiywqRxpzHjP9QSrOx8p4aZHuqloJaaLf4skzC+9mqzOkOEl6ynyhhWb171qfAcHK097Kj/if2RNowsFw5FSOyUmFvJ3BvjOsDbsnfG33SU+xXEfDm3OwDrelUsDAuP3bKK68ZUBEbEDRzm4DtauZwfeH3aqMQ8g6DYDr5ZMUsUIglftHmD5lxY/2TiygoBdb0bw3hGQrFJTuaaEnzcbZq3XBlo2LHp3Cag8COj025zP48vyZR5hjvYkjBS/ptwdPS1wMKl35usLfiz9piBlQjMBV5HHMv52D91sI5hprQCBsBYZ7ansUFbekfGdoD8fZmSfGp/fKNYUPjSg0Mmq3JgjwnGMmd7cAQXorY6uhQD2+zdR3fga16GIcqUYRiIJWMmFGB5gMbHeF+1BNpqX2PXKJ6liBDmWZwr0GWA1skBA5bkqgveOU5S2gHBYrO0aQZm/MnJTgOXC5vIU8adFzzMZZwH+sD5NP7FQdZm1p1mtEyZ5fjaM/bN4uO4S8tNgkyQ8urAKUgOlRdaMaDjFRLyBA2ts1hED5C7P5b9YsiteYOm3ZI3Prg10HM6zKZpX4griGONcGHpwW1hBuYr3cLzE4d6RBJ8K7386bMdQjtwiYSlHMKacZ2S+Kp9dLv4U4yTlS0pKp0CfAQ3soIDKgwjwYyN49/tXLB4GR1Deh8VNbnnXIHhSiCnZsgY4XUqsxYlPoMx05adaxAavjJNvBCLz6ZaH8by342t20Um/fwH1w9PRfExs0n7jmwcke8aofxhZrcZn1xewKB5u5HHH10QshhcDfVd5Ym4gLkxs7g+xxRgJ3XIU/fxMR9eCYvEvTTIG+rSkgrviCw6vt4pYZXYoyhG0p/Wq/D/cER/isiSMtWJSxcjdEnZAmIN0QxcCn0TSwJE9BSdAm0n4vrRo2JcGWC6YxB0TaJ1dl2Ox4tuEeHkj2zu4zhHiX6nXQco5XX9f/CpYfViMnCpurYGaaRMQ2nhaxMmUi41P/8Dq7i3JHk+jb43/HyY2eGkRXCxulYoXSLgHQWNztt1lSyjzqBzvJ7YYIzG41OKgc4SjayHBwT5NMH/4DqrteoxaSazlyHIl1xuUIofV+AzWIh8G+K67Sfyu73NISuu/pF9+T2zGjXGBPjgW+H+QcNwZLougsWCzt5+RvSMo69ti/e9CAsoDBCSePcD6Qvs7P+Nv5yyojAgQu2jG3yJOCU5YTy+gvB1KWnAOFxuA1Red6mzUsrCnzhFFQcVIdmDJ8XlB83IeBop1Lzk5oLrDgv1exwv36xxuvhoBi5cPHG7iHwy4nec9zNurhDXFUbqQz0Ie159OJfWl0EE2KpeHclgzzi9x43ujfJXuOV0CyxFkfDm8hPrCvvQSI7+q8TkO0y8Sv65wWfta5yZYa/WQlIf/aQZSDA6DtGGh4SJQTkgnBQh+uV0gZXlS3OTk9g3SBq4QQTmhtQ8t288hRnYMpCxY6OIU8+9i/Sqhlc5qKl+UYmR+HUeWSfk7WIchAOJOBdZvGSnK8f58K6IXc3SdDbTtkfwKi6IoYiOeZwGNjdrfxFqC/l6VB6G4NiB7zCsON15XF1j2cZLdmrA9COb6fsD9Ke+aAUHqizplhnUKlFZwcc4azy+L5e86uqYvQ0nRScJnuuRLdd0euHTAOmCeEp9pas733beAfuvDQhxBr3GYBsX2igXV9cpi3YPqWQqdI/6Tdsydo/3HHsevsPLDlApabQQ9xanObkb+qAMI/E1xkoDggVO0YosSLDL6B/AiMOCfUfCzN6tM1jYWEpvoRFnE86NT/6AbZSxCofWGr/qhYn0VF9aJCqkxXbjAnFfw+/1MbEDCrlJdUdtBxxu43jys9VBk+bCAKMtd6nrPz4b2lPdUfwmd1IseF/Yr+F301/nF5zPhoGHNgp5npQLe0fU5ynd0u2s9GthG6i+O6ujcEsq+rKNxdJvAF8Q7OnhG3y68HXUs+8pBWXHK3jvl2I3sVLfmHEdGZCzvkxJHSmS8o4sdj433iPsYX0nBYdTbOcp+la7XfII17bse5yZk0TpOlZA+2iCU+H/V/lWvHI9IMRYZWd/32yUr8drYLse7PqXMtQMGj2660IZ7AYKMwYrjNLFmu0UtmC+UMNKsra2bKio38skDElYQNGQXeLWgZ4ei8ZeqAIESEgG8Zvbcvv9cwjuG4gdp2s5UJcm62n921/K8WaAiqpqcmkAR6oMjtG58PRc2R7/IUb7FjTxf0jspOk07FCMfF9APBhf0PLDYGuf5eZ7WMSvL3PlFO4XRVoFtltbSxXWe+vlSygncDUXznTnLjrSS8wW+oV1RFe95nhN9ZDdP5ZtLFWOTHPU39JnlE94bJ9mv6++uyFnHWRUjrwey6UrCCo6UV5Vyo/iP11FNkfVGznK/q+tg32zpef0zVZ/lZFUW98pZ3q46JsKleFiC/d9UKc7q/O0c73rBAPrfrjne8zkSaFbd3xS8ydo2gGeGRc0zVGzklp8H1I7Rua4u6LmfKGnR8PsA3vk0nTRCaYP3JFBguMy0U7n5Gu352Z7MqGhD4OYXSnwnxxbcLxbTDbpv67j1C3oeuLk87vl5sOHbOsPCvf1J4f2BbpbuyVk/Zaaf3zln2f8UwWYWm86HxI0lxuIOy4XDwx0qFBOuBIcHSYJ74tDyHfmxBUBWoNj7LmN5P1MFTSzspmO0y3eGMaCoGGaDJZ+lSKUUoajGgdOJBa0nsN58RPcbA3TtDYseHERWs2RuMwSAchOx8pCKHdbWI1Pc8/KC3jvKmNXa9RMpzu2oHrvkeLewXA0yw9i8BSzCKgUnzCEExTyCio1cMr4gzXRSflfQc49SbXkZHBJR+3hZrOue7/uMldppNnfS99WmmXYZWA2T8nDPzwZ3x5NSlmuDdgvrMuS0gvvFcgU91w4FPQ/Mhw8vSKmY1Ox6EfmpNR4OOjYJdKN0aA4FLiwR1i2x7FiTZbXkxQZ89Ug2s+c4asfPO1qLrKqb4XprnjzWkRc3uP9mVeaUy3I8D643RrIfYu4hcXGmhzESAYx9u4tvKvmtpyrlhoLqu7fWT5FrC7TnV1Qhj74Bq+WDxVp3IC4jDmVgRYyAqYi1mcUz4C1xF+MtyZiT1drpG13vlc0eOd7nw1Ke21pDijbRPyeAZ15eynUBiF0uknICX1ZjYykuZs67Up6JKWIcjIigbUD5ifgqu0sxLmsvtdOc496whGtvBu3aWq2omC9/TqDUweLthAT1PbyA8l5YcL9Yu6D38IsCnwknXRMLGsc7N1DSbC7VTzOvlnCBH/z7ORZrZYL3cXLGsiMY+kwSB3s7nB8Q5DyrRRcUqyc2WA9CKbi/5DvV/0yqn/Di9P2gKv19rK5tsrKN5LOkO03iAsHpn/YwRn6vh1Ku17vz6Ca+TGvu2SSfuzGstb6W5tnX4F1vVGCb3VkVrlnKikC4uwbQ7w7KUd/fiP+AwZkHk9cLbnyYADYs+blnVs0qlRzZZEgg7XdBXRQV+exbldhmHwu8XdwtNuaKqDa7KEXOKFVUIPhtNT/dCyRfysRqwLe5KLeiF3QjMVhsYL4lVSGAwLcINvltgmvA+uSfBZT1Lik2PfueBb2DUwp8ppm0LRfxXFfKTzNQIf4IsmJcKrUD5C0Q+EYpq2vl1gGUfbWMyv5fSDzM42HteZokMy/vofM4rAwandjeWnEYkjeY9ds6Zi8mM5IjXF7ju3ktNo7JeWCEtUZXiYuVxJ9b5Q16fRdg8/2E+LWsOFx+eojXTdcPu+t66U7Jn+1wR2mefc2BBbfXv+Us768D6HOn5XyGxUMcSPYvqQFCe75Iyc++h1DBkTVjRghaPld+ylk2/2Wdyv0z4HaBRWaXdqcRwwIo1yuSLz1pPe4tQakMqyX41qcJ0PkXLe/FBY3tyxTUHzoX2CfgejJLgX19YWkcOd+VfKiKGGQDOF9sYO16J1n7SfhsrKdqaceKToHMbfelLHuZ1oxZgem7a6tCuCHBKhmm9XCJWlqsVQhcS3CS/kdJFoMG5fpDu/K6Cu4OBcmn4tdaI+/aaLzWWWzs53Gc/FLb1hBJH/8LSontdL6q1+aHOyzvU0b+LdYi6iJVhLSPEzXYQZ2f3gT7mvNLaKt5++gFJfc1WDLnzQi4W2gDCE6FniyxIQ6V/NGG8zBA3Pr2tYpgkC07lRs24NeWWAdl+aDDBWxkYO0BViF71SjvNSWX7VuHpzzVQIC60F3yrtSywmLmsoLuuWlB/QGBu94q6JmwUViq4P4eYiyskF1oKkF7fznls+0bUPn3Tln2f0a4ie0tflNtj9SDnA9TjtPv1xjDni2gf/2n3QFDFtZ2NC895fFQwSd/9PyOftBDkV9rXS9RpZ66q3J7fVWwPSCNYxsdKbUtiMpyp0yq9Llc4t3TPCHFZyFaVBVteQ805i2xn+3uoO5vCW3w2C6ABnl2ic/fQe9PZUe6CWGdANruuSXXwxVSXoyVcwJqDzC3XaVOWRGodkKJ5du5gPdxQsD99ZqKdjpzgYqRfxXUF7YuuD6LNl3tphulUNoTTrh6RLRBghI/qTvBR4FtArGpSpqGGhmyNpY4OTawMfMW3ehWw/dhDOZTF8kJbndYpjMibVdFZU2BKxasMBDsE0FB4XJzpyqV3pJkbq7fy4yg/lcUVO7Ruv8ZJG6s5LpGqhzBe1qohPbpKkbduSX1r27ixrV/YoEHaQ2ZJZAFF0zK8vjFwj9uM51ckfrnQZXbJFlgmq2Fyo60gemSpFdCDAT4OCIIJgJXwfRwLkdt968B1AMCey5fUt91Yb7masE0d4Oy4uTr8ZLK99uC3gf6wz0B9tWz5cexPopUjEAZ5juI2UD5qVmwb4Fry1IlzNUPBdCeEOS4f2SbI1iFJo1zdExgZcfBTdI4Mw9KfiuDsujmeCOfJwvYH6S+u9yx4i/Q+zNiY6blBfOyy9hXk/UwpkeEbetECX9NjfFphwrlgm8rW8RgQyyNZT3U98wS12HzfY76XFpwuDxO3B1WFx2EFRZG5zt8D4jdNG8IA8ba4j7vd55F9KYJXwbM4g8Q61sFTR9MHr+T6j5743QQqHeS0kcnJCo93JyYrqKbxE+1fU3Vjttm0vq0akoPUMVCWk31cQHVxT9KXIwi4vyYkp4b6RE3TPHu1i+hrMcW/D4Wl/yB+VzKX6u0zSIVI5AvPE7YCGD4Xkl1+44UfzqP2BEPlNieEPx36Ug33rcleL4vS1ogN2Ij+Wl2rTKVwL6Yr+Tx80NJFnR3XfHjyoq1bN6Yewt63lSjjIMja1dYoxwlxWTHy7ohXLvdIYuPd4g1+A26z+rnuc6h3EEK3bES9l7mOiNzlNAmcUj8seNnGVvgHAAF6Wni5+BlrTIHCwSOvCiwRjpcbICsNgUI/L0Q9R7uPifoZvsT1a6m9Z3EbzaJXKscgiCo2Qp16hGDbtLUXXiH3+p7x7uFT+gWukDqLtV9Hg+X4tLyJs2EMqDEfnx0wkWzy0jnJ6oyMS1nFljGA6ScGDjrSv2AekW1yf1q9J+iFSNtcTkQrHgfHc+7q8Im6fvppPNVdz1RgE/rww5PW/KYId+k89O8Wrczid/AnYgJVoaV538l/Aw09dglwbxxmpQfN6sasF54MMGmfoDEzwaqTC3aSuQOsVliko5HrzkuwxMJ799Rx/UuFWPinLpuulzyxytIOr9AQfI7Xa/3qBj7uor7zG8uQL/eo8SDpFqW8nC7WbRdWV0qRn7Q9TVSfy8rxVv8bKGHCKHtY1D3viygOmgf6NSur86qh4k3emyHOIhGRsCdK/ZR3bQMnVQ6NpjnOlR8r638XSv24whejbikvrIxwiIXsbJW0r7QTe/fViav4ws6ycQAGywWYEeq0uZ11YJNcqTt/UBqR2xfS4pLKxqzXF+nUw2SfBkUpmqbxDt/UaxFELTOsEBBBpz9A1OKtMkJJU74GCT+JP5jeMAi6yo90co6MGESurSA05ey04CvLsWnP6+05BlUp2xlKEYqF2mj9VTg0goleD16qfLvNv3dKL1OSP1/kvaPZ3X++psuSnweavyrIMXQGJ2Le0nc4FTwowZtc/mAy39kg/d0rTQPa0pxliNYr/xa0meY+53DjQwCGc+d4J4Yuw/T/ni5KnMwz4wsaS0/VccHZJ17Tstzgx5edQ+0ba2mSp0QDtROluox6lwoRjAfDRUbEH/2kpVV2ExfLPUznBUdjP8kyR9sthZYI+OQ9+86T1+mfQOHykUF6Z+k+yiMD3AVukSsex3WJshOWM/yc5AeYp6i5Ue8m1v0WmOkOC+T8TpnQ3l4ndbnn33vt/4qrbmxP6BOndxNxUfDibBWxH5oFa/0eO8ROpmEWC/QyPcpecLfw9NpEQZz+H6v4aicPXSwHeNhoXGlhJFCum1yfLDANjhCteyN2mGZipH2E3eSoLjzezid9S3fOewv9YBP+qvi70QNm53Nm2Sz3UkXVrWe90IpL/16EpbQg51aC8gtpbnA8z7isY9O1PVe1lhB2Lzmdb/+RpW+SZktkrHw6YDm4VqKZWwOy8hG2TaurtegbWVVjLyn8/vqAdb77iUeGLW5mkBBMdDzc24c8F6lTbatU/5DAy/7eJ8avK9adHMPbXHXGvXya/FnHtQM8nqdk1AEWp3QovUyWYrPVFENWNbcqhuzPM/zvW64zvZ4irqjWKsgF5MdFBBbBbgQ6KJa9689tj2cdD8qyeNddJcwghxC9k5Q3v5SjHm4axlcUBubUxf5rhQk6E+wfDlGysu45YvVpbrl6RSJI25CrWwPr0i8QVfrMYuecLqMIzRV6+vnjubbrAHQh0r6zH59IhkL3xe/FnOugNX8DZLPyjmNwLIGcc8aWSqkVYyMVWUUggbPEXidd9Zyvlpwm3xOislOKKqkDr2P1ov7dkzo5ffh74prwizzNGlNcFIJn75PaiiMHpH4ou4XBSIQ/7KOlvHsFq6bN3Sh9H0AZUGGpj3FBgNdVpKZDI7QBeibqmhAqrnRnssJBSUio2+iirWk/e4HXXw9r5v8mwJvGzgdOloXwjM7uuZkXVzDxPCSlHW+v5Sfbhvv8FxVVNcDZsAInLdgRGPBWJ1f3ynwnvBfRmY3xGgYoH0paawTHJIgENxb2p9ubtIxGtZqMMtt72p2q46XYwMv/zba3ystW7BQhEn+H5t4bl1EN5Rwd14s4zXe0jkawYvhujLOUdngmnGW/tmItlgiN+lvsrTfP0YwFiJLGMz2R0bSvjBmwpIcVn7ze5gLoARATIZ/1th3VFOMXCCNg5Z/ote9McIxG/PVEWIV0qs4XBdVMlrXs1jLIknChIKeDS7Ch0myzJ1lcboqi6qB9fg+AZd9og/FSFddHKwjrQkW5DuJ9VmvxvX6Ofkx2PAjC8TdVT7rokqTfVq4fjABbqWb1VCYUyf9FcX6MCNOAE6BMa5M1InjO93Ava0D5biSyorJBNYpMG3upxNlT5kRz+Z7Vd58qgtclHV4ZG1kWx1b1pNkPuXVwMnQy9rOruewRKqwlPYlKEVxgggrod6qKJmqC0T081Ha97Fwf138K0JDAMqj89r9HzKR3B5B2TF2P6obiTYm6Hv+oAXeHQ60NtLnX0ZswOMeusn6/0WzzhMjtX1/qMqIez3OF5infqXjOuax2XXemqJ96n2ZceCAMXsyh6ggwToJ7okr6/g5V8brYIx9XKyrFQ5aH9B9R5r2dJmut6vxnO7hoAx5MfI6Rz+BwndTnbeW1P6TlXG6RnpO6/4RHRNIE+FDMYINx7viP11TqEBrD/P2P9T4HBuXy6X5zIjz8pRO/JOqfAZl2wu6WGlVMPhCS3xh4OVsi0g9NZLxr2PFYqNZWE0XYUvqn3NpH8KfsO75TtsTFvaf66YHmRre0o3RtxyOSAY6NVk/ysIA3bS0BWHHBmM7KU8hnJbfi7Ua6KrvEj7z27bge8T7g7sG3G3m0D9RH1+rtClHilT2Yc2ITApz6vv5XsfwYVomEgddVDmCubm/kYWMLCz2MANrkj66LoELzlfa1kbovyE4uHgox5gCxQiUt4jJMUXXBIircI/297ubtD1Bwbms7iP6q6De59X67q51M0b71Uit7+/034gZ84yukX5gM6ZiJG2nRzqfjSKvmzEVG720JkvIHDCkxmfofM/rpoXM4FRdlFVjfp38Z27h+oHCaHWdFAlJCpQhvXUh3bbgGiMzoopj4p/AaiLECehfA3UzjY0srGWGR1R+BOBcQddxUATA4ucTvlZCvI4Z8+n83EE38B10Y962KR/j8H6ddP8xn27wO+saAG4z01qs7vvpWN1R9xfYn43T+h6r72Acm2hr0cHTdRGo7w6pHYQ0ZJDmET6abcFj0WEQSwEmcNskvAYWQvBVrWXSiKwZB7P5/T/QTiMF6ms1Pt9K30mXFNe7Xhelou0QcTG2iLiOkBFlTzYVQgghhBBCCIkH+K/B/QEaybZo3dB+QgsH8+23VBDBGz5yD4u1tEDAvOel+Ei0Q8WattVKqYf4CTAp/VaSpfasl7JpkOTP7NFMcm+DtnS8VI/2Xy2l6CFiFVnt86rjJGxdsX6TZTzjZ6rcwP0fVHlCZgRt+0D7BtpOZT52/D8CLfbhkEIIIYQQQggh8QGfOUT8htkWAlv1F+vThews86rMo5u+2XXzOpP+bmiBm1akD0yaFx2Ryxuld4PvZ710bR10U0yliI118IsGdX6nJEvhtl6C9wcF140lPCPSx3bX+/dRmUP7ANrefNo32vxNl1DB/3fmUEIIIYQQQgghrQeibw8rYNP6qW5A04Ao840sGM5pcI3firWiaXXFyEdSPzo33GCGN7jGl0bWTPH+oJy4o6DngzvV79mdCSGEEEIIIYRk4SrPm1YoN87LUC4ER7q3wbWfaHANWAaMoGLkf+4l9UBas0ZuRydmeIfIKT++gOdDOr+l2JUJIYQQQgghJEw6Blw2RE7u7/keiMR8RYbfIUVbmztGLdrchGqBDfOjLd7+kBnj6gbfQVqz7nU+R1yOWzLcG+5TzxTwjHCFYQYiQgghhBBCCAmUkBUjS4uNReJ7Y/5Cxt8ijd3UOp/DqmS5Bte4qsXb33CxQUjrUS8gLkA+93cz3Bv52+8q4BkRH2QZDjWEEEIIIYQQEiYhK0ZgKdDX8z065KiDRgExEcdihQbfGSo2W0mrggwtkxt8Z/kGn0O5NSXj/ScU9JzLBN7XCCGEEEIIIaRlCXmzBqWC72wcbRlDsoA0xB3qfA5XoFUaXGOkWJecVgTKjGsbfAfKpfkTXGdqxjJMK+hZl0rwHIQQQgghhBBCSiBUxQiUCosXcJ8uYtMIZ6GvlrMe/Rt8BxvzG1q07T1t5K0G3xlkZJYG30FGm+4Zy1BUGly0sSWEEEIIIYQQQkhwhKoYGaDimx5GDsrwO6SQ3TLB9+YwskiD77xh5JUWbHuXS2NLD7jRzNrgO8gus3TGMmxZ0LPmUcARQgghhBBCCPFIqIoRuNEsWNDzb2tktpS/w0Z8+wTfg1XJyg2+g6wql7VYuxtl5P4E31sxQRuFtchWGcoAN6fBBT4zlDwdOOQQQgghhBBCSFiEqhjBJrJTQfdC5ptzjXRL+P2FjVwoydw3YFnSKCMJ4pwgM8uEFmp3txv5PMH3kloNHWxkmxT3x3v5g9TPduOagVI/fTMhhBBCCCGEEPL/IFvJ9ILlViNzSu1TfSiRljTyVMrrwlVmdd2M1wIuPbeU8MxlyZAG778tcO2nKa75pZF1E7StPkauL+GZkT1nTXZtQgghhBBCCCGNQEyOYSVt2D8ycqoqQBAfZHYVbNL/ZeSrjNeFu8xLRg4xMq9Ut1T4WYsoRd400q/Gu4dLEwKuXmzkO7HBadNcG1YoJxhZzMjMYmN7tElvI5saeaLEZ/8ZuzchhBBCCCGEkEYgIObUkjfv4418oooSyA8Orz3CyEVG1lHlSxuIqfJhCyhGTm73vqHAQGBSBMF90dG7Rxrk64z8w8iZRs428noGRYtrOV3CTpFNCCGEEEIIISQADpXWcSl53MiBMiOV66lN/rywnFlMn3U+I5uLtQ4Z1yLv+26xliuEEEIIIYQQQgKhc4Bl6pHz96PFxgnpFUH9r6kCKxIEdEX8C1hMdGrS9garjeXEKkT2FJt9KCY+k9puQEnoFWifI4QQQgghhJCWJcRNGmJQ4HQ9aWpTpH59z8i7Rt428rxYd4UTI9p4I47JMfr3ydK8ipH+Rm6MsNwInAprnnvExptBpqFFVPqnuA7a6DgOO4QQQgghhBASDh0CLBOUAmeJTcFajW/FZnqB9QEUIq/o379p9z24aiBg6tZ8zSQHiDFztNhMNu3b6dJiLWDgCoWAvVCYLFbjOl8YWU9sYGFCCCGEEEIIIYHQIdByweXg10Y2MNJTbLaR18Rak7yjm8tRCa6zklgLEkKycq7YwLBJWFwFliQDxQaVhWvYy2KVfS+wOgkhhBBCCCGEpGEuIwsYmSXj79eS1gnkSvEjCA7bLWP76yvpXG0IIYQQQgghhBRM6IEgv875e6ZGJXlB/JesMV++YPURQgghhBBCSNg0u+Jgbr5ikpPprAJCCCGEEEIIaV6aXTHSla+Y5GQmoeURIYQQQgghhDQt3PARUh+40nRmNRBCCCGEEEJIc9LsipHefMUkJ3SlIYQQQgghhJAmptkVI934iomDNtSB1UAIIYQQQgghzUmzK0am8RWTnMwpdKUhhBBCCCGEkKalY5M/26x8xSQnVK4RQgghhBBCSBPTzIqRTpI/xshkFRIvI3P+HtYidKUhhBBCCCGEkCalmRUjCJo5JeNvpxo5x8imKlexqUTHi0b2NLK5kV8ZeSPjdeYQq2QjhBBCCCGEEEKiAif9Z4tVkCSV0UaONzKo3bW6GtlOrFvFdErQgnd0o5H5273DhYzsZeSRDNfrx+5ECCGEEEIIISQ24P5wZMLN77tGjjCyVINrfkTFQ/Ayycjudd7hXEa2MnJzwusNMzI7uxMhhBBCCCGEkBiB5cdkqW0J8IyRfSSZRUBPI/dS8RC8jJWfWvxUYxYjaxq5TH9T63qwOurKrkQIIYQQQgghJEYQQwVxJhCA8weV0argGCLpstZgc/xPKh6ClxFi44IkpZuRRY2cYeRLI9+LjU0zzsgNYi1MCCGEEEIIIYQ0Ka2SbWN+VYTgeR8T6x6RJdvMIaocIeGCIKsrZHi/aBtQgqwjNh7JU0aeFBuIlxBCCCGEEEIIIYaNhRYZoQusPDqyqRJCCCGEEEIISQI3kOlA8NVprIag+UCsgoQQQgghhBBCCGkIFSPpQNyJT3NeA1lTPmRV/oSvjXzi4DrvCxUjhBBCCCGEEEISQsVIOhCY890cv3/LyC5GtjbySyPXCi1QXjRykpHtjWxr5Ioc14JC5HU2U0IIIYQQQgghxA9djPxLssW+uEZs9pP23COtGw9kgpFlqtTJwWKzB6W9HhRX87CZEkIIIYQQQggh/jg0w+b/kDrX266FFSND69TL6mIzCKW5HjLS9GITJYQQQgghhBBC/LFZio36E0ZWaXC9vmLjlrSaUgQuRD9vUDdzG/m7pMtIMzObKCGEEEIIIYQQ4o+BRqY22KDj89N1Y9+ITkb+4UjZcKeR2428ZOQ7x0qM14w8aORGsbFS8l7zG7FKoUZ0MLKTzMg2U09O1vokhBBCCCGEEEKIJ3obua/O5hzBWXcQG48kKWs7UDT8XqwbCWROsYFMXSlGrjQyrz57TyNriE1dnOeal0i64L9LGLlVrJKm1jXXZfP8P/bu3uXmMIwD+GV5iGSRkqQkBhYTG2XwN1CeOiWjxcBgUhYDg0EGgzIyyCQTyWZSXgaJxcuinpKXyXV3P8PjPOec39sh9PnUd71+55zfma7u+7oAAADg9ysbVJZi9eDPOzF5wGqTzZkn0f80x8UJTYZycuJ6DG+KfFpuSow7knk/oO7RHr9TuSZzNvN5Qr3bYb4IAAAA/DGHMw8zr6I2NUbR7ZTIuDPRr8FQruFMuz6yJ4Y1L0pOz/jMZe1wn+0xz6M2g/raH3XLT/ntX0adQ7LRXxIAAAD+XWV2Sdlg06XBcDOzvqHuqejfFHkc9frMLGWrzteOdc+HWSAAAADACusyt6J9c6Fc21nbou6mzIPo3hT5HvVUTBuLmW8t6/6I2gQCAAAA+MXxaNdcKNthtnSoeyDzJbo1Rq5Gt+GobU+m3Mts8KoBAACAcduieS3t02i3BnjclWjfFHnX8xnnWtRe9JoBAACAacog0VlDS3f1rFtW+L6Odo2R0YDPf2lG3TIIdrtXDAAAAEyzNfM2VjcVykmSvQNrn4g6O2RWU+R+ZmHAM9bE9NMpI68XAAAAaFLW7N7IvIl6reVaZvcc6pZNMHdjelPkY+bgnJ5zMvMs8yHzKHMsatMEAAAAoFHZUrNjOQtzrLsvJp9IKbkw5+9QZqbsjHqNBwAAAOCvcCjzIrO0Ipej3fpfAAAA+G/8FGAAEL05oF/97sEAAAAASUVORK5CYII=\"\n");
    printf ("alt=\"\">\n");
    printf ("</div>\n");
    printf ("<div class=\"center des\">\n");
    printf ("<p><a href=\"//github.com/dani-garcia/vaultwarden\" target=\"_blank\">Vaultwarden</a>服务器全局配置</p>\n");
    printf ("</div>\n");
    printf ("<form method=\"post\" action=\"\"> \n");
    printf ("<textarea name=\"textcontent\" id=\"textcontent\" class=\"textconfig\">\n");
    Content();
    printf ("</textarea><br><br>\n");
    printf ("<div class=\"center\">\n");
    printf ("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"保存\" />\n");
    printf ("<button class=\"button\" onclick=\"toMantPage()\">管理</button>\n");
    printf ("</div>\n");
    printf ("</form>\n");
    printf ("</div>\n");
    printf ("</body>\n");
    printf ("</html>\n");
    printf ("<script>\n");
    printf ("var prol = window.location.protocol;\n");
    printf ("var domain = document.domain;\n");
    GetWebPort();
    printf ("function toMantPage() {\n");
    printf ("window.open(url,'_blank')\n");
    printf ("}\n");
    printf ("</script>\n");
    return 0;
}

int main(int argc, char **argv)
{
    char user[256];
    printf("Content-type: text/html\n\n");
    if (IsUserLogin(user, sizeof(user)) == 1) 
    {
      WebPage();
    } 
    else 
    {
        printf("<head> <meta charset=\"UTF-8\"> </head>\n");
        printf("用户未登陆。\n");
    }
    return 0;
}


