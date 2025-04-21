# 📚 Criptografia de arquivos em sistemas baseados em Debian

> Esse projeto tem como objetivo entender como funciona: chamada de sistemas, criptografia de arquivos e criação de requisição HTTP.

---

## 🧠 O que você vai aprender aqui

Este projeto foi documentado como uma mini-aula. Ao ler este README, você entenderá:

- Como o projeto está estruturado
- O papel de cada função
- Como executar o projeto do zero

---

## 🏗️ Estrutura do Projeto

```bash
c_application/
└── main.c

java_application/
└── cript/
    ├── src/
    │   └── main/
    │       ├── java/com/api/cript
    │       │    └── controller/
    │       │    ├── DTO/
    │       │    ├── model/
    │       │    ├── repo/
    │       │    └── service/
    │       └── CriptApplication.java
    └──

```

## 🔍 Entendendo o funcionamento

Começaremos pelo core da aplicação em C: a função enF.

```c
void enF(const char *filename, const unsigned char *key, const unsigned char *iv)
```

A assinatura da função é dada pelos parametros `filename`, `key` e `iv` onde são, respectivamentes, o nome do arquivo que sera criptografado, a chave que será gerada e um valor de inicialização (initialization vector).

---

```c
    FILE *input = fopen(filename, "rb");
    if (!input)
    {
        perror("!!");
        return;
    }

    char encryptedFile[256];

    snprintf(encryptedFile, sizeof(encryptedFile), "%s.locked", filename);
```

Criamos um ponteiro que aponta para a estrutura `FILE` e chamamos a função `fopen` na qual localizará (ou criará) o arquivo e abrirá em modo leitura binária (read binary).

A função `snprintf` serve para formatar uma string retornando o número total de caracteres escritos.

No primeiro parametro da função passamos o novo local em que queremos armazenar essa nova string (`encryptedFile`), no segundo parametro identificamos a quantidade maxima de caracteres que a string pode ter (256 bytes), no quarto parametro colocamos com o que a string vai ser concatenada (no nosso caso será acrescentado um .locked no nome do novo arquivo)

---

```c
    FILE *output = fopen(encryptedFile, "wb");
    if (!output)
    {
        perror("!!");
        fclose(input);
        return;
    }
```

Nesse bloco de código, começamos criando um ponteiro `output` para manipular o arquivo `encriptedFile`, esse que será criado com permição de escrita binária quando a função fopen for chamada. Todas as funções de manipulação de arquivos são dados pela blibioteca stdio que trata da entrada de dados em C.

Apartir daí começamos a utilizar funções da biblioteca openssl. A biblioteca openssl é uma biblioteca de software aberto que implementa protocolos de criptografia como SSL (Secure Sockets Layer) e TLS (Transport Layer Security)

Para entendermos como esse programa trabalha precisamos primeiro entender como a parte de criptografia funciona no geral.

A criptografia se dividi em dois grupos (os principais) e uma função de hash criptográfica (esse ultimo tem esse nome pois uma criptografia é necessariamente reversível, situação não apresentada no hash):

>

    🔐 Simétrica

    Quando a mesma chave é utilizada tanto para criptografar quanto para descriptografar
    Nesse tipo de criptografia, as duas partes precisam saber da chave secreta.
    O texto simples é transformado em uma versão codificada (texto cifrado) e apenas quem tem a chave e sabe o tipo de algoritmo utilizado consegue descriptografar.

    - AES (Advanced Encryption Standard) – o mais usado hoje, seguro e rápido.

    - DES (Data Encryption Standard)

    - 3DES

    - RC4, RC5, RC6 – outras variações

>

    🔓 Assimétrica

    Na criptografia assimétrica utiliza-se duas chaves, uma publica e uma privada.

    A chave publica pode ser distribuída livremente enquanto que a chave privada deve ser mantida em segredo. A chave pública serve para criptografar os dados enquanto que a chave privada tem o papel de descriptografar.
    Nesse tipo de criptografia sempre se gera um par de chaves que são relacionadas matematicamente mas que uma não pode ser calculada a partir da outra, além disso uma vez criptografado com a chave pública apenas quem detem a chave privada consegue visualizar a mensagem original.

    - RSA – Algoritmo tradicional baseado na fatoração de números grades

    - ECC – Elliptic Curve Cryptography

    - DSA – Digital Signature Algorithm (Utilizado para assinaturas digitais)

    - ElGamal – Algoritmo baseado no problema do logaritmo discreto.

>

    ✍️ Hash
    Uma função de hash é um tipo de algoritmo que transforma qualquer tipo de dado em uma sequencia fixa de caracteres.
    Um hash é unidirecional, ou seja, não da para voltar ao conteúdo original. Ao contrario dos outros tipos de criptografia, a função hash é deterministica, isso quer dizer que se colocarmos a mesma entrada, o hash será o mesmo.

    - MD5 – (considerado inseguro)

    - SHA-1 – (já foi quebrado)

    - SHA-256 / SHA-3 – utilizados ainda nos dias atuais

    - bcrypt, scrypt, Argon2 – utilizados em criptografia de senhas que vão ser armazenadas em banco de dados

---

No nosso programa, utilizamos o algoritmo de criptografia simétrica AES de 256 bits no modo de operação CBC.

>

    Os modos de operação definem como os blocos se encadeiam.

    ECB - Nesse modo, cada bloco é criptografado isoladamente

    CBC - Cada bloco faz uma operação XOR com o IV(initialization vector)

    CFB - Transforma o bloco em fluxo, fazendo criptografia nos bits.

---

```c
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    unsigned char buffer[1024];
    unsigned char encrypted_buffer[1024 + BLOCK_SIZE];
    int bytes_read, encrypted_bytes;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0)
    {
        EVP_EncryptUpdate(ctx, encrypted_buffer, &encrypted_bytes, buffer, bytes_read);
        fwrite(encrypted_buffer, 1, encrypted_bytes, output);
    }

    EVP_EncryptFinal_ex(ctx, encrypted_buffer, &encrypted_bytes);
    fwrite(encrypted_buffer, 1, encrypted_bytes, output);

    EVP_CIPHER_CTX_free(ctx);
    fclose(input);
    fclose(output);

    remove(filename);
    printf("Encrypted file %s\n", encryptedFile);

```

Quando escrevemos `EVP_CIPHER_CTX *ctx` estamos criando um contexto do tipo EVP_CIPHER_CTX que é um struct e representa tudo o que a biblioteca Openssl precisa saber. É com ele que vamos montar toda a criptografia.

A partir do momento que atribuímos ao ponteiro a função `EVP_CIPHER_CTX_new();` estamos necessariamente alocando memória (dinâmica) na nossa heap, nesse local será armazenado o algoritmo usado, a chave, o IV, os dados intermediários, etc.

`   EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);`
Nessa Linha ocorre a inicialização do contexto, no primeiro parâmetro passamos o contexto criado, no segundo parâmetro passamos o algoritmo e o modo, o terceiro parâmetro é a entrada para uma engine específica no qual permitira utilizar implementações dos algoritmos, ou seja, se tivermos bibliotecas de terceiros (como bibliotecas de criptografia de fabricantes ou certificada por alguma autoridade) ou dispositivos físicos(como um HSM) passamos aqui, no quarto parâmetro passamos o ponteiro para a chave secreta, e no quinto parâmetro é o nosso vetor de inicialização.

`   while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0)`

Criamos uma variavel chamada `bytes_read` que recebe o retorno da função `fread` (faz a leitura do arquivo). O primeiro parametro dessa função mostrando aonde deve ser salvo, o segundo parametro se trata da quantidade de blocos que devem ser lidos (no nosso caso 1 byte, ou seja, um caracter por vez), o terceiro parâmtro trata-se do tamanho total onde vamos guardar o que foi lido, e o quarto parâmetro mostra qual arquivo vamos ler(esse input é o ponteiro inicial para o arquivo original).

Isso se resume à: enquanto existir alguma coisa pra ler ( > 0) vamos ficar presos no while.

`EVP_EncryptUpdate(ctx, encrypted_buffer, &encrypted_bytes, buffer, bytes_read);`

Quando chamamos essa função, estamos necessariamente criptografando os dados. Ela funciona assim:
o primeiro parâmetro é o contexto com o qual construimos e especificamos o tipo de criptografia, o segundo parâmtro é aonde vamos guardar os dados criptografados, o quarto parametro nos da o retorno de quanto bytes foram necessariamente criptografados, o quinto parametro é onde vamos buscar os dados que queremos criptografar, e o sexto parametro é o tamanho de dados que queremos cifrar.

Percebe-se que ao criarmos o `encrypted_buffer` colocamos ele com o espaço de 1024 mais um BLOCK_SIZE de 16 totalizando 1040, isso se dá por conta do padding.

Quando abordamos criptografia é comum falarmos em padding. Padding é o preenchimento que ocorre para ajustar o tamanho dos dados. No padrão AES, cada bloco tem 16 bytes (128 bits) e caso a mensagem não tiver o tamanho multiplo de 16 então completa-se com bytes extras antes da criptografia. Mesmo 1024 sendo multiplo de 16 ainda é necessário o incremento de 16 bytes indicando o fim da criptografia.

`   fwrite(encrypted_buffer, 1, encrypted_bytes, output);`

Na linha acima, estamos chamando a função que escreve em arquivos. No primeiro parâmetro estamos passando o local onde vai ser lido, o segundo parametro é a quantidade de blocos que irão ser lido por vez (1 byte), o quarto parâmetro é a quantidade de bytes que serão escritos, e o quinto parâmetro é onde iremos escrever.

A função `EVP_EncryptFinal_ex(ctx, encrypted_buffer, &encrypted_bytes);` finaliza o processo de criptografia (pega o que foi criptografado e completa o último bloco com padding caso precise). No primeiro parâmetro é o contexto, o segundo parâmetro é onde os dados criptografados ficarão, o terceiro é onde fica o retorno de quantos bytes foram criptografados.

Em `    fwrite(encrypted_buffer, 1, encrypted_bytes, output)`como detalhamos anteriormente, aqui é o processo de escrita após o padding e a finalização serem feitos.

Em ` EVP_CIPHER_CTX_free(ctx);`, `fclose(input);`, `fclose(output);`  
estamos liberando memória do contexto e dos ponteiros para os arquivos.

Em `remove(filenam);` apagamos o arquivo original.
