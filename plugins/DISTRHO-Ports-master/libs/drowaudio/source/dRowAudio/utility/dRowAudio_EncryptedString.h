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

#ifndef __DROWAUDIO_ENCRYPTEDSTRING_H__
#define __DROWAUDIO_ENCRYPTEDSTRING_H__

#if JUCE_MODULE_AVAILABLE_juce_cryptography

//==============================================================================
/** 
    This class contains some simple methods to encrypt and decrypt Strings.
    
    This uses RSA encryption and can convert between a 64-bit encoded string
    and optionally Hex.
    This is a quick and useful way of encrypting passwords etc.
 
    @code
        RSAKey publicKey, privateKey;
        RSAKey::createKeyPair (publicKey, privateKey, 128);

        String encryptedString (EncryptedString::encrypt ("hello world!", publicKey.toString()));
        // encryptedString will now contain an encrypted base-64 string you can store
 
        String decryptedString (EncryptedString::decrypt (encryptedString, privateKey.toString()));
        // decryptedString equals "hello world!"
    @endcode
 */
class EncryptedString
{
public:
    //==============================================================================
    /** Encrypts a String with a given public key.
     
        This will output a base-64 string you can store safely.
        
        @param stringToEncrypt  The input string to encrypt.
        @param publicKey        The public key created from RSAKey::createKeyPair().
        @param resultAsHex      If this is true the result will be a base-16 hex
                                string. If false, the defaul base-64 encoding is used.
        @returns                The encrypted string.
     
        @see RSAKey
     */
    static String encrypt (const String& stringToEncrypt, const String& publicKey,
                           bool resultAsHex = false);

    /** Decrypts a String that was encrypted using the encrypt method.
     
        @param encryptedString  The input string to decrypt.
        @param privateKey       The private key created from RSAKey::createKeyPair().
                                This must be the private key partner of the public
                                key that was used to encrypt the string.
        @param inputIsHex       Make sure this is true if the resultAsHex option was
                                used when encrypting the String.
        @returns                The decrypted string.
     
        @see RSAKey
     */
    static String decrypt (const String& encryptedString, const String& privateKey,
                           bool inputIsHex = false);

private:
    //==============================================================================
    EncryptedString();      // don't instantiate this object, just use its static methods!
    ~EncryptedString();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EncryptedString);
};

#endif // JUCE_MODULE_AVAILABLE_juce_cryptography

#endif  // __DROWAUDIO_ENCRYPTEDSTRING_H__