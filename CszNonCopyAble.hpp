#ifndef CszNOCOPY_HPP
#define CszNOCOPY_HPP

namespace Csz
{
	class NonCopyAble
	{
		protected:
#if __cplusplus >= 201103L
			NonCopyAble()= default;
			~NonCopyAble()= default;
#else
			NonCopyAble(){}
			~NonCopyAble(){}
#endif
#if __cplusplus >= 201103L
			NonCopyAble(const NonCopyAble&)= delete;
            NonCopyAble(const NonCopyAble&&)= delete;
			NonCopyAble& operator=(const NonCopyAble&)= delete;
            NonCopyAble& operator=(const NonCopyAble&&)= delete;
#else
		private:
			NonCopyAble(const NonCopyAble&);
            NonCopyAble(const NonCopyAble&&);
			NonCopyAble& operator=(const NonCopyAble&);
            NonCopyAble& operator=(const NonCopyAble&&);
#endif
	};
}

#endif
