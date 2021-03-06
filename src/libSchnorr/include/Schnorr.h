/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ZILLIQA_SRC_LIBSCHNORR_INCLUDE_SCHNORR_H_
#define ZILLIQA_SRC_LIBSCHNORR_INCLUDE_SCHNORR_H_

#include <openssl/bn.h>
#include <openssl/ec.h>

#include <array>
#include <boost/functional/hash.hpp>
#include <memory>
#include <string>
#include <vector>

/// Specifies the interface required for classes that are byte serializable.
class SerializableCrypto {
 public:
  /// Serializes internal state to destination byte stream.
  virtual bool Serialize(std::vector<uint8_t>& dst,
                         unsigned int offset) const = 0;

  /// Deserializes source byte stream into internal state.
  virtual bool Deserialize(const std::vector<uint8_t>& src,
                           unsigned int offset) = 0;

  /// Virtual destructor.
  virtual ~SerializableCrypto() {}
};

/// Stores information on an EC-Schnorr private key.
class PrivKey : public SerializableCrypto {
  bool constructPreChecks();

 public:
  /// The scalar in the underlying field.
  std::shared_ptr<BIGNUM> m_d;

  /// Default constructor for generating a new key.
  PrivKey();

  /// Constructor for loading existing key from a byte stream.
  PrivKey(const std::vector<uint8_t>& src, unsigned int offset);

  /// Copy constructor.
  PrivKey(const PrivKey& src);

  /// Destructor.
  ~PrivKey();

  /// Returns PrivKey from input string
  static PrivKey GetPrivKeyFromString(const std::string&);

  /// Implements the Serialize function inherited from SerializableCrypto.
  bool Serialize(std::vector<uint8_t>& dst, unsigned int offset) const;

  /// Implements the Deserialize function inherited from SerializableCrypto.
  bool Deserialize(const std::vector<uint8_t>& src, unsigned int offset);

  /// Assignment operator.
  PrivKey& operator=(const PrivKey&);

  /// Equality comparison operator.
  bool operator==(const PrivKey& r) const;
};

/// Stores information on an EC-Schnorr public key.
class PubKey : public SerializableCrypto {
  bool constructPreChecks();
  bool comparePreChecks(const PubKey& r, std::shared_ptr<BIGNUM>& lhs_bnvalue,
                        std::shared_ptr<BIGNUM>& rhs_bnvalue) const;

 public:
  /// The point on the curve.
  std::shared_ptr<EC_POINT> m_P;

  /// Default constructor for an uninitialized key.
  PubKey();

  /// Constructor for generating a new key from specified PrivKey.
  PubKey(const PrivKey& privkey);

  /// Constructor for loading existing key from a byte stream.
  PubKey(const std::vector<uint8_t>& src, unsigned int offset);

  /// Copy constructor.
  PubKey(const PubKey&);

  /// Destructor.
  ~PubKey();

  /// Returns PubKey from input string
  static PubKey GetPubKeyFromString(const std::string&);

  /// Implements the Serialize function inherited from SerializableCrypto.
  bool Serialize(std::vector<uint8_t>& dst, unsigned int offset) const;

  /// Implements the Deserialize function inherited from SerializableCrypto.
  bool Deserialize(const std::vector<uint8_t>& src, unsigned int offset);

  /// Assignment operator.
  PubKey& operator=(const PubKey& src);

  /// Less-than comparison operator (for sorting keys in lookup table).
  bool operator<(const PubKey& r) const;

  /// Greater-than comparison operator.
  bool operator>(const PubKey& r) const;

  /// Equality operator.
  bool operator==(const PubKey& r) const;

  /// Inequality operator.
  bool operator!=(const PubKey& r) const;

  /// Utility std::string conversion function for public key info.
  explicit operator std::string() const;
};

// hash for using PubKey
namespace std {
template <>
struct hash<PubKey> {
  size_t operator()(PubKey const& pubKey) const noexcept;
};
}  // namespace std

using PairOfKey = std::pair<PrivKey, PubKey>;

std::ostream& operator<<(std::ostream& os, const PubKey& p);

/// Stores information on an EC-Schnorr signature.
class Signature : public SerializableCrypto {
  bool constructPreChecks();

 public:
  /// Challenge scalar.
  std::shared_ptr<BIGNUM> m_r;

  /// Response scalar.
  std::shared_ptr<BIGNUM> m_s;

  /// Default constructor.
  Signature();

  /// Constructor for loading existing signature from a byte stream.
  Signature(const std::vector<uint8_t>& src, unsigned int offset);

  /// Copy constructor.
  Signature(const Signature&);

  /// Destructor.
  ~Signature();

  /// Implements the Serialize function inherited from SerializableCrypto.
  bool Serialize(std::vector<uint8_t>& dst, unsigned int offset) const;

  /// Implements the Deserialize function inherited from SerializableCrypto.
  bool Deserialize(const std::vector<uint8_t>& src, unsigned int offset);

  /// Assignment operator.
  Signature& operator=(const Signature&);

  /// Equality comparison operator.
  bool operator==(const Signature& r) const;

  /// Utility std::string conversion function for signature info.
  explicit operator std::string() const;
};

std::ostream& operator<<(std::ostream& os, const Signature& s);

/// Implements the Elliptic Curve Based Schnorr Signature algorithm.
class Schnorr {
  /// Stores the NID_secp256k1 curve parameters for the elliptic curve scheme
  /// used in Zilliqa.
  class Curve;
  static const std::unique_ptr<Curve> m_curve;

  Schnorr();
  ~Schnorr();

 public:
  /// Public key is a point (x, y) on the curve.
  /// Each coordinate requires 32 bytes.
  /// In its compressed form it suffices to store the x co-ordinate and the sign
  /// for y. Hence a total of 33 bytes.
  static const unsigned int PUBKEY_COMPRESSED_SIZE_BYTES = 33;

  /// Returns the EC curve used.
  // const Curve& GetCurve() const;
  // static const Curve* GetCurve();

  static const EC_GROUP* GetCurveGroup();
  static const BIGNUM* GetCurveOrder();

  /// Generates a new PrivKey and PubKey pair.
  static PairOfKey GenKeyPair();

  /// Signs a message using the EC curve parameters and the specified key pair.
  static bool Sign(const std::vector<uint8_t>& message, const PrivKey& privkey,
                   const PubKey& pubkey, Signature& result);

  /// Signs a message using the EC curve parameters and the specified key pair.
  static bool Sign(const std::vector<uint8_t>& message, unsigned int offset,
                   unsigned int size, const PrivKey& privkey,
                   const PubKey& pubkey, Signature& result);

  /// Checks the signature validity using the EC curve parameters and the
  /// specified PubKey.
  static bool Verify(const std::vector<uint8_t>& message,
                     const Signature& toverify, const PubKey& pubkey);

  /// Checks the signature validity using the EC curve parameters and the
  /// specified PubKey.
  static bool Verify(const std::vector<uint8_t>& message, unsigned int offset,
                     unsigned int size, const Signature& toverify,
                     const PubKey& pubkey);

  /// Utility function for printing EC_POINT coordinates.
  static std::string PrintPoint(const EC_POINT* point);
};

#endif  // ZILLIQA_SRC_LIBSCHNORR_INCLUDE_SCHNORR_H_
