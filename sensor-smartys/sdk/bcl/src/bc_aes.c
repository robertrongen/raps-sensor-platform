#include <bc_aes.h>
#include <bc_system.h>
#include <bc_tick.h>
#include <stm32l0xx.h>

#define _BC_AES_DATATYPE AES_CR_DATATYPE_1

static void _bc_aes_set_key(const bc_aes_key_t key);
static void _bc_aes_set_iv(const bc_aes_iv_t iv);
static bool _bc_aes_process(void *buffer_out, const void *buffer_in, size_t length);

void bc_aes_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_CRYPEN_Msk;
    // Errata workaround
    RCC->AHBENR;
}

bool bc_aes_key_derivation(bc_aes_key_t decryption_key, const bc_aes_key_t key)
{
    bc_system_pll_enable();

    AES->CR = AES_CR_MODE_0;

    _bc_aes_set_key(key);

    AES->CR |= AES_CR_EN;

    bc_tick_t timeout = bc_tick_get() + 100;

    while ((AES->SR & AES_SR_CCF) == 0)
    {
        if (timeout < bc_tick_get())
        {
            bc_system_pll_disable();
            return false;
        }
    }

    decryption_key[0] = AES->KEYR0;
    decryption_key[1] = AES->KEYR1;
    decryption_key[2] = AES->KEYR2;
    decryption_key[3] = AES->KEYR3;

    AES->CR |= AES_CR_CCFC;

    bc_system_pll_disable();

    return true;
}

bool bc_aes_ecb_encrypt(void *buffer_out, const void *buffer_in, const size_t length, const bc_aes_key_t key)
{
    if ((length % 16) != 0)
    {
        return false;
    }

    AES->CR = _BC_AES_DATATYPE;

    _bc_aes_set_key(key);

    return _bc_aes_process(buffer_out, buffer_in, length);
}

bool bc_aes_ecb_decrypt(void *buffer_out, const void *buffer_in, size_t length, bc_aes_key_t key)
{
    if ((length % 16) != 0)
    {
        return false;
    }

    AES->CR = _BC_AES_DATATYPE | AES_CR_MODE_1;

    _bc_aes_set_key(key);

    return _bc_aes_process(buffer_out, buffer_in, length);
}

bool bc_aes_cbc_encrypt(void *buffer_out, const void *buffer_in, size_t length, bc_aes_key_t key, bc_aes_iv_t iv)
{
    if ((length % 16) != 0)
    {
        return false;
    }

    AES->CR = AES_CR_CHMOD_0 | _BC_AES_DATATYPE;

    _bc_aes_set_key(key);

    _bc_aes_set_iv(iv);

    return _bc_aes_process(buffer_out, buffer_in, length);
}

bool bc_aes_cbc_decrypt(void *buffer_out, const void *buffer_in, size_t length, bc_aes_key_t key, bc_aes_iv_t iv)
{
    if ((length % 16) != 0)
    {
        return false;
    }

    AES->CR = AES_CR_CHMOD_0 | _BC_AES_DATATYPE | AES_CR_MODE_1;

    _bc_aes_set_key(key);

    _bc_aes_set_iv(iv);

    return _bc_aes_process(buffer_out, buffer_in, length);
}

void bc_aes_key_from_uint8(bc_aes_key_t key, const uint8_t *buffer)
{
    uint8_t *tmp = (uint8_t *) key;

    for (int i = 0; i < 16; i++)
    {
        tmp[15 - i] = buffer[i];
    }
}

void bc_aes_iv_from_uint8(bc_aes_iv_t iv, const uint8_t *buffer)
{
    uint8_t *tmp = (uint8_t *) iv;

    for (int i = 0; i < 16; i++)
    {
        tmp[15 - i] = buffer[i];
    }
}

static void _bc_aes_set_key(const bc_aes_key_t key)
{
    AES->KEYR0 = key[0];
    AES->KEYR1 = key[1];
    AES->KEYR2 = key[2];
    AES->KEYR3 = key[3];
}

static void _bc_aes_set_iv(const bc_aes_iv_t iv)
{
    AES->IVR0 = iv[0];
    AES->IVR1 = iv[1];
    AES->IVR2 = iv[2];
    AES->IVR3 = iv[3];
}

static bool _bc_aes_process(void *buffer_out, const void *buffer_in, size_t length)
{
    uint8_t buffer[16];

    uint32_t *inputaddr = (uint32_t *) buffer_in;
    uint32_t *outputaddr = (uint32_t *) buffer_out;

    bc_system_pll_enable();

    bool inputaddr_aligned = ((uint32_t) inputaddr & (uint32_t) 0x00000003U) == 0U;
    bool outputaddr_aligned = ((uint32_t) outputaddr & (uint32_t) 0x00000003U) == 0U;

    if (!outputaddr_aligned)
    {
        outputaddr = (uint32_t *) buffer;
    }

    AES->CR |= AES_CR_EN;

    for (size_t i = 0; i < length; i += 16)
    {
        if (!inputaddr_aligned)
        {
            memcpy(buffer, (uint8_t *) buffer_in + i, 16);

            inputaddr = (uint32_t *) buffer;
        }

        AES->DINR = *(inputaddr++);
        AES->DINR = *(inputaddr++);
        AES->DINR = *(inputaddr++);
        AES->DINR = *(inputaddr++);

        bc_tick_t timeout = bc_tick_get() + 100;

        while ((AES->SR & AES_SR_CCF) == 0)
        {
            if (timeout < bc_tick_get())
            {
                bc_system_pll_disable();
                return false;
            }
        }

        AES->CR |= AES_CR_CCFC;

        *(outputaddr++) = AES->DOUTR;
        *(outputaddr++) = AES->DOUTR;
        *(outputaddr++) = AES->DOUTR;
        *(outputaddr++) = AES->DOUTR;

        if (!outputaddr_aligned)
        {
            memcpy((uint8_t *) buffer_out + i, buffer, 16);
            outputaddr = (uint32_t *) buffer;
        }
    }

    AES->CR &= ~AES_CR_EN;

    bc_system_pll_disable();

    return true;
}
