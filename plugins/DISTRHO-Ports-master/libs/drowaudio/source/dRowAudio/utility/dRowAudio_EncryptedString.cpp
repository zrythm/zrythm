/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#if JUCE_MODULE_AVAILABLE_juce_cryptography



EncryptedString::EncryptedString()
{
}

EncryptedString::~EncryptedString()
{
}

String EncryptedString::encrypt (const String& stringToEncrypt, const String& publicKey, bool resultAsHex)
{
    RSAKey rsaKey (publicKey);
    
    CharPointer_UTF8 stringPointer (stringToEncrypt.toUTF8());
    MemoryBlock stringMemoryBlock (stringPointer.getAddress(), stringPointer.sizeInBytes());

    BigInteger stringAsData;
    stringAsData.loadFromMemoryBlock (stringMemoryBlock);

    rsaKey.applyToValue (stringAsData);
    
    if (resultAsHex)
    {
        MemoryBlock encryptedMemoryBlock (stringAsData.toMemoryBlock());
        return String::toHexString ((char*) encryptedMemoryBlock.getData(), encryptedMemoryBlock.getSize(), 0);
    }
    else
    {
        return stringAsData.toMemoryBlock().toBase64Encoding();
    }
}

String EncryptedString::decrypt (const String& encryptedString, const String& privateKey, bool inputIsHex)
{
    RSAKey rsaKey (privateKey);
    
    MemoryBlock encryptedMemoryBlock;
    
    if (inputIsHex)
    {
        encryptedMemoryBlock.loadFromHexString (encryptedString);
    }
    else
    {
        encryptedMemoryBlock.fromBase64Encoding (encryptedString);
    }

    BigInteger stringAsData;
    stringAsData.loadFromMemoryBlock (encryptedMemoryBlock);
    
    rsaKey.applyToValue (stringAsData);
    
    return stringAsData.toMemoryBlock().toString();
}

//==============================================================================
#if DROWAUDIO_UNIT_TESTS

class EncryptedStringTests  : public UnitTest
{
public:
    EncryptedStringTests() : UnitTest ("EncryptedString") {}
    
    void runTest()
    {
        beginTest ("EncryptedString");
        
        // test with some known keys
        const String knownPublicKey ("11,7eb9c1c3bc8360d1f263f8ee45d98b01");
        const String knownPrivateKey ("2545b175ce0885e2fb72f1b59bbf8261,7eb9c1c3bc8360d1f263f8ee45d98b01");
        
        expectEquals (EncryptedString::encrypt ("hello world!", knownPublicKey),
                      String ("16.PcHEsm3XRKxv0V8hMeKv5A"));
        expectEquals (EncryptedString::decrypt ("16.PcHEsm3XRKxv0V8hMeKv5A", knownPrivateKey),
                      String ("hello world!"));
        
        expectEquals (EncryptedString::encrypt ("hello world!", knownPublicKey, true),
                      String ("508714ed8963d222c3b5d58bcdb7c07a"));
        expectEquals (EncryptedString::decrypt ("508714ed8963d222c3b5d58bcdb7c07a", knownPrivateKey, true),
                      String ("hello world!"));
        
        // test with some generated keys
        RSAKey publicKey, privateKey;
        RSAKey::createKeyPair (publicKey, privateKey, 128);
        
        String encryptedString (EncryptedString::encrypt ("hello world!", publicKey.toString()));
        String decryptedString (EncryptedString::decrypt (encryptedString, privateKey.toString()));
        
        expectEquals (decryptedString, String ("hello world!"));
    }
};

static EncryptedStringTests encryptedStringTests;

#endif // DROWAUDIO_UNIT_TESTS

//==============================================================================



#endif //JUCE_MODULE_AVAILABLE_juce_cryptography
