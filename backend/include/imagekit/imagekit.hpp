/**
 * @file imagekit.hpp
 * @brief Lightweight image resize and compression toolkit for C++.
 *
 * imagekit provides a minimal and modern API for resizing and
 * compressing images in C++ applications.
 *
 * Designed for:
 *  - Web backends
 *  - CDN preprocessing
 *  - Storage optimization
 *  - Edge services
 *
 * Header-only and dependency-free (core abstraction).
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace imagekit
{

  /**
   * @brief Supported output image formats.
   */
  enum class Format
  {
    JPEG,
    PNG,
    WEBP
  };

  /**
   * @brief Basic image container.
   *
   * Represents raw image pixel data in memory.
   * This abstraction does not decode image formats.
   */
  struct Image
  {
    std::vector<std::uint8_t> data;
    std::size_t width{0};
    std::size_t height{0};
    std::size_t channels{0}; // e.g. 3 = RGB, 4 = RGBA

    /**
     * @brief Returns true if the image contains pixel data.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return !data.empty() && width > 0 && height > 0 && channels > 0;
    }
  };

  /**
   * @brief Resize options.
   */
  struct ResizeOptions
  {
    std::size_t target_width{0};
    std::size_t target_height{0};
    bool preserve_aspect_ratio{true};
  };

  /**
   * @brief Compression options.
   */
  struct CompressOptions
  {
    Format format{Format::JPEG};
    int quality{85}; // 1-100
  };

  /**
   * @brief Image processing utilities.
   */
  class Processor
  {
  public:
    /**
     * @brief Resize an image.
     *
     * @param input Source image.
     * @param options Resize configuration.
     * @return Resized image.
     *
     * @throws std::invalid_argument if input is invalid.
     */
    static Image resize(const Image &input, const ResizeOptions &options)
    {
      if (!input.valid())
      {
        throw std::invalid_argument("imagekit::resize: invalid input image");
      }

      if (options.target_width == 0 || options.target_height == 0)
      {
        throw std::invalid_argument("imagekit::resize: invalid target size");
      }

      // Skeleton implementation (no real resampling)
      Image output = input;
      output.width = options.target_width;
      output.height = options.target_height;

      return output;
    }

    /**
     * @brief Compress an image.
     *
     * @param input Source image.
     * @param options Compression configuration.
     * @return Encoded image buffer.
     *
     * @throws std::invalid_argument if input is invalid.
     */
    static std::vector<std::uint8_t> compress(
        const Image &input,
        const CompressOptions &options)
    {
      if (!input.valid())
      {
        throw std::invalid_argument("imagekit::compress: invalid input image");
      }

      if (options.quality < 1 || options.quality > 100)
      {
        throw std::invalid_argument("imagekit::compress: quality must be 1-100");
      }

      // Skeleton implementation
      // Real encoding would depend on a backend (libjpeg, libpng, etc.)
      return input.data;
    }
  };

} // namespace imagekit
