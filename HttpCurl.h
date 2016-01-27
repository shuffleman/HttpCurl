
#pragma once
#include <string>
#include <curl/curl.h>
#include <pthread.h>
#include <openssl/crypto.h>
static pthread_mutex_t* lockarray;

class CHttpCurl
{
	public:
		CHttpCurl()
		{
			m_curlCode = CURLE_OK;
			m_extra = NULL;
		}
		virtual ~CHttpCurl()
		{
		}
	public:
		//初始化 curl环境 只能调用1次
		static CURLcode CurlGlobalInit()
		{
			CURLcode m_curlCode = curl_global_init(CURL_GLOBAL_ALL);
			init_locks();
			return m_curlCode;
		}
		//清理 curl环境 只能调用1次
		static void CurlGlobalCleanUp()
		{
			kill_locks();
			curl_global_cleanup();
		}
		static void lock_callback(int mode, int type, char *file, int line)
		{
			(void)file;
			(void)line;
			if (mode & CRYPTO_LOCK) {
				pthread_mutex_lock(&(lockarray[type]));
			}
			else {
				pthread_mutex_unlock(&(lockarray[type]));
			}
		}
		static unsigned long thread_id(void)
		{
			unsigned long ret;
			ret = (unsigned long)pthread_self();
			return(ret);
		}
		static void init_locks()
		{
			lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
			for(int i = 0; i < CRYPTO_num_locks(); i++) 
			{
				pthread_mutex_init(&(lockarray[i]),NULL);
			}
			CRYPTO_set_id_callback((unsigned long (*)())thread_id);
			CRYPTO_set_locking_callback((void (*)(int, int, const char*, int))lock_callback);
		}
		static void kill_locks()
		{
			CRYPTO_set_locking_callback(NULL);
			for(int i = 0; i < CRYPTO_num_locks(); i++)
			{
				pthread_mutex_destroy(&(lockarray[i]));
			}
			OPENSSL_free(lockarray);
		}
		static size_t rev_data(void *ptr, size_t size, size_t nmemb, void *stream)
		{
			CHttpCurl* pHttp = (CHttpCurl*)stream;
			size_t len = size * nmemb;
			std::string response;
			if(ptr && len)
			{
				response.assign((char*)ptr,len);
			}
			pHttp->Response(response);
			return size * nmemb;
		}
		//提交POST请求
		//url post请求的url地址
		//fileds post请求的表单数据
		//return CURLcode 
		CURLcode Post(std::string& url,std::string& fileds)
		{
			m_curl = curl_easy_init();
			curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(m_curl, CURLOPT_URL,url.c_str());
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,fileds.c_str());
			if(url.find("https://") != std::string::npos)
			{
				curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
				curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
			}
			curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 3L);
			curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 3L);
			curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, rev_data);
			curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
			m_curlCode = curl_easy_perform(m_curl);
			curl_easy_cleanup(m_curl);
			return m_curlCode;
		}
		//提交GET请求
		//url get请求的url地址
		//return CURLcode
		CURLcode Get(std::string& url)
		{
			m_curl = curl_easy_init();
			curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(m_curl, CURLOPT_URL,url.c_str());
			if(url.find("https://") != std::string::npos)
			{
				curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
				curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
			}
			curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 3L);
			curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 3L);
			curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, rev_data);
			curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
			m_curlCode = curl_easy_perform(m_curl);
			curl_easy_cleanup(m_curl);
			return m_curlCode;
		}
		//return CURLcode 
		CURLcode GetCURLcode()
		{
			return m_curlCode;
		}
		//设置附带数据
		void SetExtra(void* extra)
		{
			m_extra = extra;
		}
		//获取附带数据
		void* GetExtra()
		{
			return m_extra;
		}
		//post/get 请求的相应函数
		//response 收到的相应数据
		virtual void Response(std::string& response) = 0;
	private:
		CURLcode m_curlCode;
		CURL* m_curl;
		void* m_extra;
};
