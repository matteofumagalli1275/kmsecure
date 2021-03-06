#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tinydir.h"
#include "kmsecure.h"
#include "aes.h"

using namespace std;
bool to_crypt = false;
char algorithm[20] = {0};
char* key = NULL;
char* directory = NULL;
std::vector<char*> ext_to_ignore;
kmsecure kms;
kmsecure::kmsecure_info info;
ikmcrypto* kmcrypto;

void explore(char*);

int main (int argc, char **argv)
{
    int c;
    info.hard = true;
    info.soft_perc = 0;
    info.soft_point = 0;

    while ((c = getopt (argc, argv, "vhsc:d:l:p:a:r:i:")) != -1)
        switch (c)
        {
            case 'v':
                printf("version %d.%d\n",KMS_VERSION/10,KMS_VERSION % 10);
                return 0;
            case 'c':
                to_crypt = true;
                key = optarg;
                break;
            case 'd':
                to_crypt = false;
                key = optarg;
                break;
            case 'h':
                info.hard = true;
                break;
            case 's':
                info.hard = false;
                break;
            case 'r': //Directory (r of resource)
                directory = optarg;
                break;
            case 'i': //similar string to ignore (extensions for example)
                ext_to_ignore.push_back(optarg);
                break;
            case 'l':
                if(info.hard == false)
                {
                   info.soft_point = atoi(optarg);
                }
                else
                {
                    printf("-p and -s parameters require soft mode\n");
                    return -1;
                }
                break;
            case 'p':
                if(info.hard == false)
                {
                   info.soft_perc = atoi(optarg);
                }
                else
                {
                    printf("-p and -s parameters require soft mode\n");
                    return -1;
                }
                break;
            case 'a':
                strncpy(algorithm,optarg,20);
                break;
            case '?':
                if (optopt == 'c' || optopt == 'd' || optopt == 'r' || optopt == 'p' || optopt == 'l'
                         || optopt == 'a' || optopt == 'i')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
        default:
          return -2;
        }

    if(directory == NULL)
    {
        printf("-r (directory) needed\n");
        return -3;
    }

    if(key == NULL)
    {
        printf("no key was given");
        return -4;
    }

    if(algorithm[0] == 0)
        kmcrypto = new Blowfish();
    else if(strncmp(algorithm,"Blowfish",20) == 0)
        kmcrypto = new Blowfish();
    else if(strncmp(algorithm,"AES128",20) == 0)
        kmcrypto = new AES128();
    else if(strncmp(algorithm,"AES256",20) == 0)
        kmcrypto = new AES256();
    else
    {
        printf("No valid algorithm provided");
        return -5;
    }

    kmcrypto->set_key(key,strlen(key));

    kms.set_crypto(kmcrypto);
    explore(directory);

    return 0;
}

void explore(char* str_dir)
{
    tinydir_dir dir;
    tinydir_open(&dir, str_dir);
    tinydir_file file;
    ofstream out_file;
    ifstream in_file;
    char* buffer;
    int size;
    bool file_to_ignore;

    while (dir.has_next)
    {
        tinydir_readfile(&dir, &file);

        if(file.is_dir && strcmp(file.name,".") != 0 && strcmp(file.name,"..") != 0)
        {
            printf("%s\n",file.name);
            explore(file.path);
        }
        else if(!file.is_dir)
        {
            file_to_ignore = false;
            for(unsigned i=0;i<ext_to_ignore.size();i++)
            {
                if(strstr(file.name,ext_to_ignore[i]) != 0)
                {
                    file_to_ignore = true;
                    break;
                }
            }
            if(!file_to_ignore)
            {
                in_file.open(file.path,ios::binary);
                in_file.seekg(0,ios::end);
                size = in_file.tellg();
                in_file.seekg(0);

                if(size > 0)
                {
                    printf("working on %s ...\n",file.path);
                    buffer = new char[size];
                    in_file.read(buffer,size);
                    in_file.close();
                    if(to_crypt)
                        kms.crypt(&buffer,size,info);
                    else
                        kms.decrypt(&buffer,size);
                    out_file.open(file.path,ios::binary);
                    out_file.write(buffer,size);
                    out_file.close();

                    delete[] buffer;
                }
            }

        }
        tinydir_next(&dir);

    }
}

