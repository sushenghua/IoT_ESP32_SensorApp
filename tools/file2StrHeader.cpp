/*
 * CmdSetUtility utility methods
 * Copyright (c) 2016 Shenghua Su
 *
 * build:  g++ -std=c++11 -o f2sh file2StrHeader.cpp -lssl -lcrypto
 * export: ./f2sh -i inputfile -o outputfile
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// using namespace std;

// Ref: https://gist.github.com/barrysteyn/7308212

// size_t calcDecodeLength(const char* b64input) { //Calculates the length of a decoded string
//   size_t len = strlen(b64input),
//     padding = 0;

//   if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
//     padding = 2;
//   else if (b64input[len-1] == '=') //last char is =
//     padding = 1;

//   return (len*3)/4 - padding;
// }

// int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) //Decodes a base64 encoded string
// {
//   BIO *bio, *b64;

//   int decodeLen = calcDecodeLength(b64message);
//   *buffer = (unsigned char*)malloc(decodeLen + 1);
//   (*buffer)[decodeLen] = '\0';

//   bio = BIO_new_mem_buf(b64message, -1);
//   b64 = BIO_new(BIO_f_base64());
//   bio = BIO_push(b64, bio);

//   BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
//   *length = BIO_read(bio, *buffer, strlen(b64message));
//   assert(*length == decodeLen); //length should equal decodeLen, else something went horribly wrong
//   BIO_free_all(bio);

//   return (0); //success
// }

// int Base64Encode(unsigned char* buffer, size_t length, char** b64text) //Encodes a binary safe base 64 string
// {
//   BIO *bio, *b64;
//   BUF_MEM *bufferPtr;

//   b64 = BIO_new(BIO_f_base64());
//   bio = BIO_new(BIO_s_mem());
//   bio = BIO_push(b64, bio);

//   BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
//   BIO_write(bio, buffer, length);
//   BIO_flush(bio);
//   BIO_get_mem_ptr(bio, &bufferPtr);
//   BIO_set_close(bio, BIO_NOCLOSE);
//   BIO_free_all(bio);

//   *b64text=(*bufferPtr).data;

//   return (0); //success
// }


// Ref: https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption

typedef const EVP_CIPHER * (*CipherAlgorithm)(void);

void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
  abort();
}

int encrypt(unsigned char *plaintext, int plaintextLen, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext, CipherAlgorithm cAlgorithm)
{
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertextLen;

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher. The IV size for *most* modes
   * is the same as the block size. For AES this is 128 bits
   */
  if (1 != EVP_EncryptInit_ex(ctx, cAlgorithm(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintextLen))
    handleErrors();
  ciphertextLen = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
    handleErrors();
  ciphertextLen += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertextLen;
}

int decrypt(unsigned char *ciphertext, int ciphertextLen, unsigned char* key,
            unsigned char *iv, unsigned char *plaintext, CipherAlgorithm cAlgorithm)
{
  EVP_CIPHER_CTX *ctx;
  int len;
  int plaintextLen;

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher. The IV size for *most* modes
   * is the same as the block size. For AES this is 128 bits
   */
  if (1 != EVP_DecryptInit_ex(ctx, cAlgorithm(), NULL, key, iv))
    handleErrors();

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextLen))
    handleErrors();
  plaintextLen = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintextLen += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintextLen;
}


// Ref: https://stackoverflow.com/questions/5288076/base64-encoding-and-decoding-with-openssl

namespace {
  struct BIOFreeAll { 
    void operator()(BIO* p) { BIO_free_all(p); } 
  };
}

void base64Encode(const std::vector<unsigned char>& binary, std::string &encoded)
{
  // const std::vector<char> charVect(binary.begin(), binary.end());
  // charVect.push_back('\0');
  std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
  BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
  BIO* sink = BIO_new(BIO_s_mem());
  BIO_push(b64.get(), sink);
  BIO_write(b64.get(), binary.data(), binary.size());
  BIO_flush(b64.get());
  const char* encodedP;
  const long len = BIO_get_mem_data(sink, &encodedP);
  encoded = std::string(encodedP, len);
}

// Assumes no newlines or extra characters in encoded string
void base64Decode(const std::string& encoded, std::vector<unsigned char> &decoded)
{
  std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
  BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
  BIO* source = BIO_new_mem_buf(encoded.c_str(), -1); // read-only source
  BIO_push(b64.get(), source);
  const int maxlen = encoded.length() / 4 * 3 + 1;
  // std::vector<unsigned char> decoded(maxlen);
  const int len = BIO_read(b64.get(), decoded.data(), maxlen);
  decoded.resize(len);
}

CipherAlgorithm getCipherAlgorithm(std::string &cipherAlgorithm)
{
  CipherAlgorithm cAlgorithm = NULL;

  if (cipherAlgorithm == "aes_128_ecb") {
    cAlgorithm = EVP_aes_128_ecb;
  }
  else if (cipherAlgorithm == "aes_128_cbc") {
    cAlgorithm = EVP_aes_128_cbc;
  }
  else if (cipherAlgorithm == "aes_256_ecb") {
    cAlgorithm = EVP_aes_256_ecb;
  }
  else if (cipherAlgorithm == "aes_256_cbc") {
    cAlgorithm = EVP_aes_256_cbc;
  }

  return cAlgorithm;
}

unsigned char _ccache[4096];

bool encryptStringToBinary(const std::string &plainText, std::vector<unsigned char> &cipherBinary,
                   std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  bool succeeded = true;
  CipherAlgorithm cAlgorithm = NULL;

  if (cipherAlgorithm == "none") {
    std::copy(plainText.begin(), plainText.end(), std::back_inserter(cipherBinary));
    // cipherBinary.push_back('\0');
  }
  else {
    cAlgorithm = getCipherAlgorithm(cipherAlgorithm);
    succeeded = cAlgorithm != NULL;
  }

  if (cAlgorithm != NULL) {
    unsigned char *plaintext = (unsigned char *)plainText.c_str();
    unsigned char *keys = (unsigned char *)key.c_str();
    unsigned char *ivs = (unsigned char *)iv.c_str();



std::cout << "----------------------begin-----------------" << std::endl;
std::cout << "input: " << plainText << std::endl;
std::cout << "key:   " << keys << std::endl;
std::cout << "iv     " << ivs << std::endl;
std::vector<unsigned char> _plaintCache(2048, 0);

//     int len = encrypt(plaintext, plainText.length(), keys, ivs, cipherBinary.data(), cAlgorithm);
//     cipherBinary.resize(len);

// std::cout << "en:    " << cipherBinary.data() << std::endl;
// std::cout << "ensize:" << cipherBinary.size() << std::endl;
// unsigned char *cipherdata = cipherBinary.data();
// int dlen = decrypt(cipherdata, cipherBinary.size(), keys, ivs, _plaintCache.data(), cAlgorithm);

    int len = encrypt(plaintext, plainText.length(), keys, ivs, _ccache, cAlgorithm);

std::cout << "en1:   " << (const char*)_ccache << std::endl;
std::cout << "ensize:" << sizeof((const char*)_ccache) << std::endl; 
int dlen = decrypt(_ccache, len, keys, ivs, _plaintCache.data(), cAlgorithm);

_plaintCache.resize(dlen);
std::string pt((const char*)_plaintCache.data(), _plaintCache.size());

std::cout << "de:    " << pt << std::endl;
std::cout << "match: " << (pt == plainText) << std::endl;
std::cout << "---------------------- end -----------------" << std::endl;
  }

  return succeeded;
}

std::vector<unsigned char> _plaintCache(2048, 0);

bool decryptBinaryToString(std::vector<unsigned char> &cipherBinary, std::string &plainText, 
                           std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  bool succeeded = true;
  CipherAlgorithm cAlgorithm = NULL;

  if (cipherAlgorithm == "none") {
    plainText = std::string((const char*)cipherBinary.data(), cipherBinary.size());
  }
  else {
    cAlgorithm = getCipherAlgorithm(cipherAlgorithm);
    succeeded = cAlgorithm != NULL;
  }

  if (cAlgorithm != NULL) {
    unsigned char *cipherdata = (unsigned char*)cipherBinary.data();
    unsigned char *keys = (unsigned char *)key.c_str();
    unsigned char *ivs = (unsigned char *)iv.c_str();
    int len = decrypt(cipherdata, cipherBinary.size(), keys, ivs, _plaintCache.data(), cAlgorithm);
    _plaintCache.resize(len);
    plainText = std::string((const char*)_plaintCache.data(), _plaintCache.size());
  }

  return succeeded;
}

std::vector<unsigned char> _enCipherBinaryCache(4096, 1);

bool encryptToBase64(const std::string &plainText, std::string &cipherText,
                     std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  if (encryptStringToBinary(plainText, _enCipherBinaryCache, cipherAlgorithm, key, iv)) {
    base64Encode(_enCipherBinaryCache, cipherText);
    return true;
  }
  else {
    return false;
  }
}

std::vector<unsigned char> _deCipherBinaryCache(4096, 0);

bool decryptFromBase64(const std::string &cipherText, std::string &plainText,
                       std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  base64Decode(cipherText, _deCipherBinaryCache);
  if (decryptBinaryToString(_deCipherBinaryCache, plainText, cipherAlgorithm, key, iv)) {
    return true;
  }
  else {
    return false;
  }
}

int main( int argc, const char* argv[])
{
  bool formatErr = false;

  do { // flow control

    if (argc != 7 && argc != 9 && argc != 13) {
      formatErr = true;
      break;
    }

    int flagCount = (argc - 1) / 2;
    int inputFileNameArgIndex = -1;
    int outputFileNameArgIndex = -1;
    int stringNameArgIndex = -1;
    int cipherAlgorithmArgIndex = -1;
    int keyArgIndex = -1;
    int ivArgIndex = -1;

    for (int i = 0; i < flagCount; ++i) {

      const char *flag = argv[1 + i * 2];
      int argIndex = 2 + i * 2;

      if (std::string(flag) == "-i")
        inputFileNameArgIndex = argIndex;
      else if (std::string(flag) == "-o")
        outputFileNameArgIndex = argIndex;
      else if (std::string(flag) == "-n")
        stringNameArgIndex = argIndex;
      else if (std::string(flag) == "-c")
        cipherAlgorithmArgIndex = argIndex;
      else if (std::string(flag) == "-key")
        keyArgIndex = argIndex;
      else if (std::string(flag) == "-iv")
        ivArgIndex = argIndex;
      else {
        formatErr = true;
        break;
      }
    }
    if (formatErr) break;

    if (inputFileNameArgIndex == -1 || outputFileNameArgIndex == -1 || stringNameArgIndex == -1) {
      formatErr = true;
      break;
    }

    std::string cipherAlgorithmName = "none";
    std::string cipherText;
    std::string key;
    std::string iv;
    std::string plainText;

    if (cipherAlgorithmArgIndex != -1) {
      cipherAlgorithmName = argv[cipherAlgorithmArgIndex];
      if (cipherAlgorithmName != "none") {
        if (keyArgIndex == -1 || ivArgIndex == -1) {
          formatErr = true;
          break;
        }
        else {
          key = argv[keyArgIndex];
          iv = argv[ivArgIndex];
        }
      }
    }

    // file stream
    std::fstream fi(argv[inputFileNameArgIndex], std::fstream::in);
    std::fstream fo(argv[outputFileNameArgIndex], std::fstream::out);
    std::fstream fc("checkresult", std::fstream::out);
    std::string linestr;
    std::string upercaseName(argv[stringNameArgIndex]);
    std::transform(upercaseName.begin(), upercaseName.end(), upercaseName.begin(), ::toupper);
    // write string name
    fo <<  "/*\n" \
         " * const char * variable header\n" \
         " * Copyright (c) 2017 Shenghua Su\n" \
         " *\n" \
         " * This file is generated by script, should not be modified\n" \
         " */\n\n";
    fo << "#ifndef _" << upercaseName << "_H" << std::endl;
    fo << "#define _" << upercaseName << "_H" << std::endl << std::endl;

    fo << "const char " << argv[stringNameArgIndex] << "[] =" << std::endl;
    // read source and write target
    while (!fi.eof()) {
      std::getline(fi, linestr);
      if (linestr.length() > 0) {
        // if (linestr.back() == '\r') linestr.pop_back();
        fo << "\"";
        if (cipherAlgorithmName == "none") {
          fo << linestr;
        }
        else {
          if ( encryptToBase64(linestr, cipherText, cipherAlgorithmName, key, iv) ) {
            fo << cipherText;
            // std::cout << cipherText << std::endl;
          }
          // if ( decryptFromBase64(cipherText, plainText, cipherAlgorithmName, key, iv) )
          //   fc << plainText;
        }
        fo << "\\n\"";
        // if (fi.peek() != EOF) 
        //   fo << "\\n\"";
        // else
        //   fo << "\\n\0\"";
      }
      else {
        fo << "\"";
        fo << "\\n\"";
      }
      if (fi.peek() != EOF) {
        fo << "\\\n";
      }
    }
    fo << ";" << std::endl << std::endl;
    fo << "#endif // _" << upercaseName << "_H" << std::endl;

    // close stream
    fi.close();
    fo.close();
    fc.close();

  } while (false); // end of flow control

  if (formatErr) {
    std::cout << "use format: " << argv[0]
              << " -i inputFile -n stringName -o outputFile [-c cipherAlgorithmName -key key -iv iv]" << std::endl;
    return -1;
  }
  else
    return 0;
}
