#pragma once

#include <glad/glad.h>
#include <cassert>
#include <cstddef>
#include <type_traits>

/**
 * @tparam T The data layout struct. std140-compatible:
 *           - standard-layout
 *           - sizeof(T) multiple of 16
 */
template<typename T>
class UniformBuffer
{
    static_assert(std::is_standard_layout_v<T>,
        "UniformBuffer<T> requires T to be standard-layout");
    static_assert(sizeof(T) % 16 == 0,
        "UniformBuffer<T> requires sizeof(T) to be a multiple of 16");

public:
    /**
     * Creates a UBO, allocates GPU memory, and binds to a binding point.
     * @param bindingPoint The uniform binding slot in shaders (layout(binding = N)).
     * @param usagePattern GL_STATIC_DRAW, GL_DYNAMIC_DRAW, etc.
     */
    UniformBuffer(GLuint bindingPoint, GLenum usagePattern = GL_STATIC_DRAW)
        : bindingPoint_(bindingPoint)
    {
        // Generate buffer handle
        glGenBuffers(1, &bufferID_);

        // Allocate memory without initializing data
        glBindBuffer(GL_UNIFORM_BUFFER, bufferID_);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, usagePattern);

        // Link buffer to binding point
        glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint_, bufferID_, 0, sizeof(T));

        // Unbind
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    /**
     * @brief Deletes the buffer when object is destroyed.
     */
    ~UniformBuffer()
    {
        if (bufferID_ != 0) {
            glDeleteBuffers(1, &bufferID_);
        }
    }

    // Disable copy, allow move
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    UniformBuffer(UniformBuffer&& other) noexcept
        : bufferID_(other.bufferID_), bindingPoint_(other.bindingPoint_)
    {
        other.bufferID_ = 0;
    }

    UniformBuffer& operator=(UniformBuffer&& other) noexcept
    {
        if (this != &other) {
            release();
            bufferID_ = other.bufferID_;
            bindingPoint_ = other.bindingPoint_;
            other.bufferID_ = 0;
        }
        return *this;
    }

    /**
     * @brief Update entire buffer data in one call.
     * @param data The struct to upload
     */
    void update(const T& data)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, bufferID_);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // Upload value at byte?offset
    template<typename U>
    void updateMember(GLintptr offset, const U& value) {
        static_assert(std::is_standard_layout_v<U>,
            "must be POD");
        glBindBuffer(GL_UNIFORM_BUFFER, bufferID_);
        glBufferSubData(
            GL_UNIFORM_BUFFER,
            offset,
            sizeof(U),
            &value
        );
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    /**
     * @brief Map buffer to client memory for direct write.
     * @param accessFlags GL_MAP_WRITE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT, etc.
     * @return Pointer to mapped memory (type T*).
     */
    T* map(GLbitfield accessFlags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, bufferID_);
        return static_cast<T*>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(T), accessFlags));
    }

    /**
     * @brief Unmap previously mapped buffer.
     */
    void unmap()
    {
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    /**
     * @brief Re-bind the buffer to its binding point
     */
    void bind() const
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint_, bufferID_);
    }

private:
    GLuint bufferID_ = 0;   // OpenGL buffer handle
    GLuint bindingPoint_;   // UBO binding slot in shaders

    void release()
    {
        if (bufferID_ != 0) {
            glDeleteBuffers(1, &bufferID_);
            bufferID_ = 0;
        }
    }
};
