#pragma once

struct NonCopyable {
	NonCopyable& operator=(const NonCopyable&) = delete;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable() = default;
};