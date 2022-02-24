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
#define CONFIGFILE "../bin/data/config.json"

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
            printf("抱歉，没有找到配置文件。</p>");
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
            system("sed -i 's/\r//g' ../bin/config.json");
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

//套件全局参数配置页面
int WebPage() 
{
    printf ("<!DOCTYPE html>\n");
    printf ("<html lang=\"en\">\n");
    printf ("<head>\n");
    printf ("<meta charset=\"UTF-8\">\n");
    printf ("<title>套件全局参数配置</title>\n");
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
    printf ("<a href=\"https://imnks.com\" target=\"_blank\">  <img src=\"data:image/png;base64,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\"\n");
    printf ("alt=\"\"></a>\n");
    printf ("</div>\n");
    printf ("<div class=\"center des\">\n");
    printf ("<p>套件全局参数配置，保存后需要停用再启动套件生效！</p>\n");
    printf ("</div>\n");
    printf ("<form method=\"post\" action=\"\"> \n");
    printf ("<textarea name=\"textcontent\" id=\"textcontent\" class=\"textconfig\">\n");
    Content();
    printf ("</textarea><br><br>\n");
    printf ("<div class=\"center\">\n");
    printf ("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"保存配置\" />\n");
    printf ("</div>\n");
    printf ("</form>\n");
    printf ("</div>\n");
    printf ("</body>\n");
    printf ("</html>\n");
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


