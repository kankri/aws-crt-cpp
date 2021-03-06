/*
 * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/crt/http/HttpRequestResponse.h>

#include <aws/crt/io/Stream.h>
#include <aws/http/request_response.h>
#include <aws/io/stream.h>

namespace Aws
{
    namespace Crt
    {
        namespace Http
        {

            HttpMessage::HttpMessage(Allocator *allocator, struct aws_http_message *message, bool ownsMessage) noexcept
                : m_allocator(allocator), m_message(message), m_bodyStream(nullptr), m_ownsMessage(ownsMessage)
            {
            }

            HttpMessage::~HttpMessage()
            {
                if (m_message != nullptr)
                {
                    aws_input_stream *old_stream = aws_http_message_get_body_stream(m_message);
                    if (old_stream != nullptr)
                    {
                        aws_input_stream_destroy(old_stream);
                    }

                    if (m_ownsMessage)
                    {
                        aws_http_message_destroy(m_message);
                    }
                    m_message = nullptr;
                }
            }

            std::shared_ptr<Aws::Crt::Io::InputStream> HttpMessage::GetBody() const noexcept { return m_bodyStream; }

            bool HttpMessage::SetBody(const std::shared_ptr<Aws::Crt::Io::IStream> &body) noexcept
            {
                aws_http_message_set_body_stream(m_message, nullptr);
                m_bodyStream = nullptr;

                if (body != nullptr)
                {
                    m_bodyStream = MakeShared<Io::StdIOStreamInputStream>(m_allocator, body, m_allocator);
                    if (m_bodyStream == nullptr || !m_bodyStream)
                    {
                        return false;
                    }
                    aws_http_message_set_body_stream(m_message, m_bodyStream->GetUnderlyingStream());
                }

                return true;
            }

            bool HttpMessage::SetBody(const std::shared_ptr<Aws::Crt::Io::InputStream> &body) noexcept
            {
                m_bodyStream = body;
                aws_http_message_set_body_stream(
                    m_message, m_bodyStream && *m_bodyStream ? m_bodyStream->GetUnderlyingStream() : nullptr);

                return true;
            }

            size_t HttpMessage::GetHeaderCount() const noexcept { return aws_http_message_get_header_count(m_message); }

            Optional<HttpHeader> HttpMessage::GetHeader(size_t index) const noexcept
            {
                HttpHeader header;
                if (aws_http_message_get_header(m_message, &header, index) != AWS_OP_SUCCESS)
                {
                    return Optional<HttpHeader>();
                }

                return Optional<HttpHeader>(header);
            }

            bool HttpMessage::AddHeader(const HttpHeader &header) noexcept
            {
                return aws_http_message_add_header(m_message, header) == AWS_OP_SUCCESS;
            }

            bool HttpMessage::EraseHeader(size_t index) noexcept
            {
                return aws_http_message_erase_header(m_message, index) == AWS_OP_SUCCESS;
            }

            HttpRequest::HttpRequest(Allocator *allocator)
                : HttpMessage(allocator, aws_http_message_new_request(allocator), true)
            {
            }

            HttpRequest::HttpRequest(Allocator *allocator, struct aws_http_message *message)
                : HttpMessage(allocator, message, false)
            {
            }

            Optional<ByteCursor> HttpRequest::GetMethod() const noexcept
            {
                ByteCursor method;
                if (aws_http_message_get_request_method(m_message, &method) != AWS_OP_SUCCESS)
                {
                    return Optional<ByteCursor>();
                }

                return Optional<ByteCursor>(method);
            }

            bool HttpRequest::SetMethod(ByteCursor method) noexcept
            {
                return aws_http_message_set_request_method(m_message, method) == AWS_OP_SUCCESS;
            }

            Optional<ByteCursor> HttpRequest::GetPath() const noexcept
            {
                ByteCursor path;
                if (aws_http_message_get_request_path(m_message, &path) != AWS_OP_SUCCESS)
                {
                    return Optional<ByteCursor>();
                }

                return Optional<ByteCursor>(path);
            }

            bool HttpRequest::SetPath(ByteCursor path) noexcept
            {
                return aws_http_message_set_request_path(m_message, path) == AWS_OP_SUCCESS;
            }

            HttpResponse::HttpResponse(Allocator *allocator)
                : HttpMessage(allocator, aws_http_message_new_response(allocator))
            {
            }

            Optional<int> HttpResponse::GetResponseCode() const noexcept
            {
                int response = 0;
                if (aws_http_message_get_response_status(m_message, &response) != AWS_OP_SUCCESS)
                {
                    return Optional<int>();
                }

                return response;
            }

            bool HttpResponse::SetResponseCode(int response) noexcept
            {
                return aws_http_message_set_response_status(m_message, response) == AWS_OP_SUCCESS;
            }
        } // namespace Http
    }     // namespace Crt
} // namespace Aws
