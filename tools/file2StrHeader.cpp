/*
 * CmdSetUtility utility methods
 * Copyright (c) 2016 Shenghua Su
 *
 * build:  g++ -std=c++11 -DDEBUG -o f2sh file2StrHeader.cpp -lssl -lcrypto
 * export: ./f2sh -i inputfile -o outputfile
 *
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h> // strncpy
#include <algorithm>
#include <memory>
#include <vector>
#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// #define DEBUG

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

void base64Encode(const unsigned char *binary, size_t size, std::string &encoded)
{
  // const std::vector<char> charVect(binary.begin(), binary.end());
  // charVect.push_back('\0');
  std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
  BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
  BIO* sink = BIO_new(BIO_s_mem());
  BIO_push(b64.get(), sink);
  BIO_write(b64.get(), binary, size);
  BIO_flush(b64.get());
  const char* encodedP;
  const long len = BIO_get_mem_data(sink, &encodedP);
  encoded = std::string(encodedP, len);
}

// Assumes no newlines or extra characters in encoded string
void base64Decode(const std::string& encoded, unsigned char *decoded, size_t &size)
{
  std::unique_ptr<BIO, BIOFreeAll> b64(BIO_new(BIO_f_base64()));
  BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
  BIO* source = BIO_new_mem_buf(encoded.c_str(), -1); // read-only source
  BIO_push(b64.get(), source);
  const int maxlen = encoded.length() / 4 * 3 + 1;
  size = BIO_read(b64.get(), decoded, maxlen);
}

CipherAlgorithm cipherAdaption(std::string &cipherAlgorithm, std::string &iv, unsigned char *&ivp)
{
  CipherAlgorithm cAlgorithm = NULL;

  if      (cipherAlgorithm == "aes_128_ecb") {
    cAlgorithm = EVP_aes_128_ecb;
    ivp = NULL;
  }
  else if (cipherAlgorithm == "aes_128_cbc") {
    cAlgorithm = EVP_aes_128_cbc;
    ivp = (unsigned char *)iv.c_str();
  }
  else if (cipherAlgorithm == "aes_256_ecb") {
    cAlgorithm = EVP_aes_256_ecb;
    ivp = NULL;
  }
  else if (cipherAlgorithm == "aes_256_cbc") {
    cAlgorithm = EVP_aes_256_cbc;
    ivp = (unsigned char *)iv.c_str();
  }

  return cAlgorithm;
}

bool encryptStringToBinary(const std::string &plainText, unsigned char * cipherBinary, size_t &size,
                           std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  bool succeeded = true;
  CipherAlgorithm cAlgorithm = NULL;
  unsigned char *ivp = NULL;

  if (cipherAlgorithm == "none") {
    size = plainText.length();
    strncpy((char*)cipherBinary, plainText.c_str(), size);
    cipherBinary[size] = '\0';
    // std::copy(plainText.begin(), plainText.end(), std::back_inserter(cipherBinary));
    // cipherBinary.push_back('\0');
  }
  else {
    cAlgorithm = cipherAdaption(cipherAlgorithm, iv, ivp);
    succeeded = cAlgorithm != NULL;
  }

  if (cAlgorithm != NULL) {
    unsigned char *plaintext = (unsigned char *)plainText.c_str();
    unsigned char *keyp = (unsigned char *)key.c_str();
    size = encrypt(plaintext, plainText.length(), keyp, ivp, cipherBinary, cAlgorithm);
  }

  return succeeded;
}

#define CACHE_SIZE 4096
unsigned char _plaintCache[CACHE_SIZE];

bool decryptBinaryToString(unsigned char* cipherBinary, size_t size, std::string &plainText, 
                           std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  bool succeeded = true;
  CipherAlgorithm cAlgorithm = NULL;
  unsigned char *ivp = NULL;

  if (cipherAlgorithm == "none") {
    plainText = std::string((const char*)cipherBinary, size);
  }
  else {
    cAlgorithm = cipherAdaption(cipherAlgorithm, iv, ivp);
    succeeded = cAlgorithm != NULL;
  }

  if (cAlgorithm != NULL) {
    unsigned char *keyp = (unsigned char *)key.c_str();
    int len = decrypt(cipherBinary, size, keyp, ivp, _plaintCache, cAlgorithm);
    plainText = std::string((const char*)_plaintCache, len);
  }

  return succeeded;
}

unsigned char _cipherBinaryCache[CACHE_SIZE];

bool encryptToBase64(const std::string &plainText, std::string &cipherText,
                     std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  size_t size;
  if (encryptStringToBinary(plainText, _cipherBinaryCache, size, cipherAlgorithm, key, iv)) {
    base64Encode(_cipherBinaryCache, size, cipherText);
    return true;
  }
  else {
    return false;
  }
}

bool decryptFromBase64(const std::string &cipherText, std::string &plainText,
                       std::string &cipherAlgorithm, std::string &key, std::string &iv)
{
  size_t size;
  base64Decode(cipherText, _cipherBinaryCache, size);
  if (decryptBinaryToString(_cipherBinaryCache, size, plainText, cipherAlgorithm, key, iv)) {
    return true;
  }
  else {
    return false;
  }
}

void ostreamHeader(std::fstream &fo, const char *name)
{
  std::string upercaseName(name);
  std::transform(upercaseName.begin(), upercaseName.end(), upercaseName.begin(), ::toupper);
  // write string name
  fo <<  "/*\n" \
       " * const char * variable header\n" \
       " * Copyright (c) 2018 Shenghua Su\n" \
       " *\n" \
       " * This file is generated by script, should not be modified\n" \
       " */\n\n";
  fo << "#ifndef _" << upercaseName << "_H" << std::endl;
  fo << "#define _" << upercaseName << "_H" << std::endl << std::endl;

  fo << "const char " << name << "[] =" << std::endl;
}

void ostreamTail(std::iostream &stream, const char *name)
{
  std::string upercaseName(name);
  std::transform(upercaseName.begin(), upercaseName.end(), upercaseName.begin(), ::toupper);
  stream << ";" << std::endl << std::endl;
  stream << "#endif // _" << upercaseName << "_H" << std::endl;
}

void inToOutProcess(std::iostream &istream, std::iostream &ostream, bool retainNewline,
                    std::string &cipherAlgorithmName, std::string &key, std::string &iv)
{
  // read source and write target
  std::string plainText;
  std::string cipherText;
  std::string linestr;
  while (!istream.eof()) {
    std::getline(istream, linestr);
    if (linestr.length() > 0) {
      // if (linestr.back() == '\r') linestr.pop_back();
      ostream << "\"";
      if (cipherAlgorithmName == "none") {
        ostream << linestr;
      }
      else {
#ifdef DEBUG
        if ( encryptToBase64(linestr, cipherText, cipherAlgorithmName, key, iv) ) {
          ostream << cipherText;
          std::cout << "plain: " << linestr << std::endl;
          std::cout << "ciper: " << cipherText << std::endl;
          if ( decryptFromBase64(cipherText, plainText, cipherAlgorithmName, key, iv) ) {
            std::cout << "plain: " << plainText << std::endl << std::endl;
          }
          else {
            std::cout << "decrypt err!" << std::endl << std::endl;
          }

        }
        else {
          std::cout << "encrypt err!" << std::endl << std::endl;
          ostream << "err line!!!";
        }
#else
        if ( encryptToBase64(linestr, cipherText, cipherAlgorithmName, key, iv) ) {
          ostream << cipherText;
        }
        else {
          ostream << "err line!!!";
        }
#endif
      }
      if (retainNewline)
        ostream << "\\n\"";
      else
        ostream << "\"";
      // if (istream.peek() != EOF) 
      //   ostream << "\\n\"";
      // else
      //   ostream << "\\n\0\"";
    }
    else {
      ostream << "\"";
      ostream << "\\n\"";
    }
    if (istream.peek() != EOF) {
      ostream << "\\\n";
    }
  }
}

void genEncryptionTrail(std::string &cipherAlgorithmName, std::string &key, std::string &iv, bool genSki)
{
  std::string keyEncoded;
  std::string ivEncoded;
  std::string mergedKeyIv;
  std::fstream fk("encryption_note", std::fstream::out);
  uint64_t mergedSecretV;
  uint32_t keySecretV;
  uint32_t ivSecretV;

  // algorithm
  fk << "algorithm:" << std::endl;
  fk << cipherAlgorithmName << std::endl << std::endl;

  // key
  base64Encode((const unsigned char*)key.data(), key.length(), keyEncoded);
  memcpy(&keySecretV, keyEncoded.data()+keyEncoded.length()-4, 4);

  fk << "key:" << std::endl
     << key << std::endl
     << "key encoded:" << std::endl
     << keyEncoded << std::endl
     << "key encoded str part:" << std::endl
     << keyEncoded.substr(0, keyEncoded.length()-4) << std::endl
     << "key encoded int part:" << std::endl
     << keySecretV << std::endl
     << "key encoded int part bit invert:" << std::endl
     << ~keySecretV << std::endl << std::endl;

  // iv
  base64Encode((const unsigned char*)iv.data(), iv.length(), ivEncoded);
  memcpy(&ivSecretV, ivEncoded.data()+ivEncoded.length()-4, 4);
  fk << "iv:" << std::endl
     << iv << std::endl
     << "iv encoded:" << std::endl
     << ivEncoded << std::endl
     << "iv encoded str part:" << std::endl
     << ivEncoded.substr(0, ivEncoded.length()-4) << std::endl
     << "iv encoded int part:" << std::endl
     << ivSecretV << std::endl
     << "iv encoded int part bit invert:" << std::endl
     << ~ivSecretV << std::endl << std::endl;

  // merged str
  mergedKeyIv = keyEncoded.substr(0, keyEncoded.length()-4) + ivEncoded.substr(0, ivEncoded.length()-4);
  fk << "merged encoded keyiv: (key str + iv str)" << std::endl
     << mergedKeyIv << std::endl << std::endl;

  // merged secret int
  mergedSecretV = keySecretV;
  mergedSecretV <<= 32;
  mergedSecretV |= ivSecretV;
  fk << "merged secret: | 32bit key | 32bit  iv |" << std::endl
     << mergedSecretV << std::endl;

  fk.close();

  // gen merged key iv header string
  if (genSki) {
    std::fstream fski("ski.h", std::fstream::out);
    std::stringstream istreamKeyIv(mergedKeyIv, std::stringstream::in);
    ostreamHeader(fski, "ski");
    std::string none("none");
    inToOutProcess(istreamKeyIv, fski, false, none, key, iv);
    ostreamTail(fski, "ski");
    fski.close();
  }
}

int main( int argc, const char* argv[])
{
  bool formatErr = false;

  do { // flow control

    // if (argc < 7 || argc > 14) {
    if (argc != 7 && argc != 9 && argc != 11 && argc != 13 && argc != 15 && argc != 17 && argc != 19) {
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
    int appendNewlineArgIndex = -1;
    int genCipherNoteArgIndex = -1;
    int genSkiArgIndex = -1;

    for (int i = 0; i < flagCount; ++i) {

      const char *flag = argv[1 + i * 2];
      int argIndex = 2 + i * 2;

      if      (std::string(flag) == "-i"    || std::string(flag) == "--input")
        inputFileNameArgIndex = argIndex;
      else if (std::string(flag) == "-o"    || std::string(flag) == "--output")
        outputFileNameArgIndex = argIndex;
      else if (std::string(flag) == "-n"    || std::string(flag) == "--name")
        stringNameArgIndex = argIndex;
      else if (std::string(flag) == "-a"    || std::string(flag) == "--appendnewline")
        appendNewlineArgIndex = argIndex;
      else if (std::string(flag) == "-c"    || std::string(flag) == "--ciphertype")
        cipherAlgorithmArgIndex = argIndex;
      else if (std::string(flag) == "-key"  || std::string(flag) == "--cipherkey")
        keyArgIndex = argIndex;
      else if (std::string(flag) == "-iv"   || std::string(flag) == "--cipheriv")
        ivArgIndex = argIndex;
      else if (std::string(flag) == "-note" || std::string(flag) == "--genciphernote")
        genCipherNoteArgIndex = argIndex;
      else if (std::string(flag) == "-ski"  || std::string(flag) == "--genski")
        genSkiArgIndex = argIndex;
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

    bool appendNewline = true;
    bool genCipherNote = true;
    bool genSki = true;
    std::string cipherAlgorithmName = "none";
    std::string key;
    std::string iv;

    if (appendNewlineArgIndex != -1) {
      if (strcmp(argv[appendNewlineArgIndex], "true") == 0) appendNewline = true;
      else if (strcmp(argv[appendNewlineArgIndex], "false") == 0) appendNewline = false;
      else { formatErr = true; break; }
    }

    if (cipherAlgorithmArgIndex != -1) {
      cipherAlgorithmName = argv[cipherAlgorithmArgIndex];
      if (cipherAlgorithmName != "none") {
        if (keyArgIndex == -1) { formatErr = true; break; }
        key = argv[keyArgIndex];

        if (cipherAlgorithmName.rfind("_ecb") == std::string::npos) {
          if (ivArgIndex == -1) { formatErr = true; break; }
          iv = argv[ivArgIndex];
        }

        if (genCipherNoteArgIndex != -1) {
          if (strcmp(argv[genCipherNoteArgIndex], "true") == 0) genCipherNote = true;
          else if (strcmp(argv[genCipherNoteArgIndex], "false") == 0) genCipherNote = false;
          else { formatErr = true; break; }
        }
        if (genSkiArgIndex != -1) {
          if (strcmp(argv[genSkiArgIndex], "true") == 0) genSki = true;
          else if (strcmp(argv[genSkiArgIndex], "false") == 0) genSki = false;
          else { formatErr = true; break; }
        }
      }
    }

    // file stream
    std::fstream fi(argv[inputFileNameArgIndex], std::fstream::in);
    std::fstream fo(argv[outputFileNameArgIndex], std::fstream::out);

    ostreamHeader(fo, argv[stringNameArgIndex]);
    inToOutProcess(fi, fo, appendNewline, cipherAlgorithmName, key, iv);
    ostreamTail(fo, argv[stringNameArgIndex]);

    // close stream
    fi.close();
    fo.close();

    if (cipherAlgorithmName != "none" && genCipherNote) {
      genEncryptionTrail(cipherAlgorithmName, key, iv, genSki);
    }

  } while (false); // end of flow control

  if (formatErr) {
    std::cout << "use format: " << std::endl
              << argv[0] << " <arguments>" << std::endl
              << " ARGUMENTS:" << std::endl
              << "  -i(--input) file                    input file name" << std::endl
              << "  -n(--name) name                     string name" << std::endl
              << "  -o(--output) file                   output file name" << std::endl
              << "  -a(--appendnewline) true|false      [optional]" << std::endl
              << "  -c(--ciphertype) type               [optional] cipher algorithm name" << std::endl
              << "  -key(--cipherkey) key               [optional] cipher key"<< std::endl
              << "  -iv(--cipheriv) iv                  [optional] cipher iv" << std::endl
              << "  -note(--genciphernote) ture|false   [optional] generate cipher note" << std::endl
              << "  -ski(--genski) true | false         [optional] cipher key"<< std::endl;
    return -1;
  }
  else
    return 0;
}
