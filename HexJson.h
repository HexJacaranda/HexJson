// HexJson.h
#pragma once
#include <gcroot.h>
#include <math.h>
#pragma warning(disable:4018)
typedef int index_t;

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace System::Reflection;

namespace HexJson {
	//前置声明
	ref class JsonArray;
	ref class JsonValue;
	ref class JsonObject;

	public enum class JsonValueType
	{
		Null,
		Boolean,
		Number,
		String,
		Array,
		Object
	};
	//Json值接口
	public interface class IJsonValue
	{
		virtual JsonValueType GetValueType();
		static bool IsValue(IJsonValue^value) {
			JsonValueType type = value->GetValueType();
			return (type == JsonValueType::Boolean || type == JsonValueType::Number || type == JsonValueType::Null || type == JsonValueType::String);
		}
	};
	//Json值
	public ref class JsonValue :IJsonValue
	{
	protected:
		String^m_string;
		double m_cache;
		JsonValueType m_type;
	public:
		virtual JsonValueType GetValueType() {
			return m_type;
		}
		double AsDouble() {
			if (m_type == JsonValueType::Number)
				return m_cache;
			throw gcnew Exception(L"Error type");
		}
		String^ AsString() {
			if (m_type == JsonValueType::String)
				return m_string;
			throw gcnew Exception(L"Error type");
		}
		bool AsBoolean() {
			if (m_type == JsonValueType::Boolean)
				return m_cache == 1;
			throw gcnew Exception(L"Error type");
		}
		Object^ GetBoxedValue() {
			if (m_type == JsonValueType::String)
				return m_string;
			else if (m_type == JsonValueType::Boolean)
				return m_cache == 0 ? false : true;
			else if (m_type == JsonValueType::Null)
				return nullptr;
			else
				return m_cache;
		}
	};
	//Json对象
	public ref class JsonObject :IJsonValue, IEnumerable<KeyValuePair<String^, IJsonValue^>>
	{
	protected:
		Dictionary<String^, IJsonValue^>^ m_map;
		template<class T>
		T TryGetAs(String^index,JsonValueType type)
		{
			IJsonValue^result = nullptr;
			bool found = m_map->TryGetValue(index, result);
			if (!found)
				return (T)nullptr;
			if (result->GetValueType() == type)
				return (T)result;
			else
				return (T)nullptr;
		}
	public:
		virtual JsonValueType GetValueType() {
			return JsonValueType::Object;
		}
		//取出值
		property IJsonValue^ default [String^] {
			IJsonValue^ get(String^index) {
			return m_map[index];
			}
		}
		//数目
		property int Count
		{
			int get() {
				return m_map->Count;
			}
		}
		//取出值,作为Value
		JsonValue^ GetValue(String^index) {
			IJsonValue^result = nullptr;
			bool found = m_map->TryGetValue(index, result);
			if (!found)
				return (JsonValue^)nullptr;
			JsonValueType type = result->GetValueType();
			if (type == JsonValueType::Boolean || type == JsonValueType::Number || type == JsonValueType::Null || type == JsonValueType::String)
				return (JsonValue^)result;
			else
				return (JsonValue^)nullptr;
		}
		//取出值,作为Object
		JsonObject^GetObject(String^index) {
			return TryGetAs<JsonObject^>(index, JsonValueType::Object);
		}
		//取出值,作为Array
		JsonArray^GetArray(String^index) {
			return TryGetAs<JsonArray^>(index, JsonValueType::Array);
		}	

		virtual IEnumerator<KeyValuePair<String^, IJsonValue^>>^ GetEnumerator()
		{
			return m_map->GetEnumerator();
		}
		virtual System::Collections::IEnumerator^ GetBoxedEnumerator() = System::Collections::IEnumerable::GetEnumerator{
			return m_map->GetEnumerator();
		}
	};
	//Json数组
	public ref class JsonArray :IJsonValue, IEnumerable<IJsonValue^>
	{
	protected:
		List<IJsonValue^>^ m_list;
	public:
		virtual JsonValueType GetValueType() {
			return JsonValueType::Array;
		}
		property IJsonValue^ default[int]{
			IJsonValue^ get(int index) {
			return m_list[index];
			}
		}
		//数目
		property int Count
		{
			int get() {
				return m_list->Count;
			}
		}
		//取出值,作为Object
		JsonObject^ GetObject(int index) {
			IJsonValue^ value = m_list[index];
			return value->GetValueType() == JsonValueType::Object ? (JsonObject^)value : nullptr;
		}
		//取出值,作为Value
		JsonValue^ GetValue(int index) {
			IJsonValue^ value = m_list[index];
			return IJsonValue::IsValue(value) ? (JsonValue^)value : nullptr;
		}
		//取出值,作为Array
		JsonArray^ GetArray(int index) {
			IJsonValue^ value = m_list[index];
			return value->GetValueType() == JsonValueType::Array ? (JsonArray^)value : nullptr;
		}

		virtual IEnumerator<IJsonValue^>^ GetEnumerator() {
			return m_list->GetEnumerator();
		}
		virtual System::Collections::IEnumerator^ GetBoxedEnumerator() = System::Collections::IEnumerable::GetEnumerator{
			return m_list->GetEnumerator();
		}
	};
	private ref class JsonObjectInstance :JsonObject
	{
	public:
		JsonObjectInstance(Dictionary<String^, IJsonValue^>^target) {
			this->m_map = target;
		}
	};
	private ref class JsonArrayInstance :JsonArray
	{
	public:
		JsonArrayInstance(List<IJsonValue^>^target) {
			this->m_list = target;
		}
	};
	private ref class JsonValueInstance :JsonValue
	{
	public:
		JsonValueInstance(String^string,double cache,JsonValueType type) {
			this->m_cache = cache;
			this->m_string = string;
			this->m_type = type;
		}
	};
	//Json解析异常
	public ref struct JsonParserException :Exception
	{
	public:
		JsonParserException(String^msg):Exception(msg){}
	};
	private class Helper
	{
	public:
		template<class CharT>
		static bool BasicStringSingleMatch(const CharT*target_string,interior_ptr<CharT> source, size_t target_length) {
			if (target_string[0] == source[0] && target_string[target_length - 1] == source[target_length - 1]) {
				for (index_t i = 1; i < target_length - 1; ++i) {
					if (target_string[i] != source[i])
						return false;
				}
				return true;
			}
			return false;
		}
		template<class CharT>
		static inline bool IsHex(CharT target_char) {
			if (target_char <= (CharT)'f'&&target_char >= (CharT)'a')
				return true;
			else
				return false;
		}
		template<class IntT, class CharT>
		static IntT HexToInt(interior_ptr<CharT> string, size_t count)
		{
			IntT ret = 0;
			IntT factor = 0;
			auto pow = [](IntT base, IntT pow) {
				if (pow == 0)
					return 1;
				IntT ret = base;
				for (IntT i = 1; i < pow; ++i)
					ret *= base;
				return ret;
			};
			for (size_t i = count - 1; i >= 0; --i)
			{
				if (IsHex(*string))
					factor = (*string - (CharT)'a') + 10;
				else if (IsInt(*string))
					factor = *string - (CharT)'0';
				else
					return ret;
				ret += factor * pow(16, i);
				string++;
			}
			return ret;
		}
		template<class CharT>
		static inline bool IsInt(CharT target_char) {
			if (target_char <= (CharT)'9'&&target_char >= (CharT)'0')
				return true;
			else
				return false;
		}
		template<class CharT>
		static size_t FloatSniff(interior_ptr<CharT> string) {
			size_t out = 0;
			short dot_times = 0;
			if (*string == '-' || IsInt(*string))
			{
				string++;
				out++;
				for (;;)
				{
					if (*string == L'.')
					{
						++dot_times;
						if (dot_times > 1)
							return 0;
					}
					else
					{
						if (!IsInt(*string))
							break;
					}
					++string;
					++out;
				}
			}
			return out;
		}
		template<class FloatT, class CharT>
		static FloatT StringToFloat(interior_ptr<CharT> string, size_t count) {
			int sign = 1;
			if (string[0] == (CharT)'-')
				sign = -1;
			FloatT int_part = 0;
			FloatT float_part = 0;
			FloatT pow_iter = 10;
			bool dot_sign = false;
			size_t iter = 0;
			if (sign < 0)
				iter = 1;
			for (size_t i = iter; i < count; ++i)
			{
				if (string[i] == (CharT)'.')
				{
					dot_sign = true;
					continue;
				}
				if (IsInt(string[i]))
				{
					if (!dot_sign)
						int_part = int_part * 10 + (FloatT)(string[i] - (CharT)'0');
					else
					{
						float_part += (FloatT)(string[i] - (CharT)'0') / pow_iter;
						pow_iter *= 10;
					}
				}
				else
					return (int_part + float_part)*sign;
			}
			return (int_part + float_part)*sign;
		}
	};
	private enum class TokenType
	{
		String,
		Number,
		Null,
		Boolean,
		ObjectStart,//{
		ObjectEnd,//}
		ArrayStart,//[
		ArrayEnd,//]
		Split,//,
		Represent,//:
		End
	};
	private struct Token
	{
		TokenType Type;
		gcroot<String^> Value;
		double ValueCache;
		Token(TokenType Type, double const&Value) :ValueCache(Value), Type(Type) {}
		Token(TokenType Type, String^Value) :Value(Value), Type(Type) {}
	};
	private ref class JsonTokenizer
	{
		array<wchar_t>^ m_source = nullptr;
		index_t m_inx = 0;
		size_t m_size = 0;
	private:
		bool IsNull()
		{
			return Helper::BasicStringSingleMatch(L"null", &m_source[0] + m_inx, 4);
		}
		bool IsTrue()
		{
			return Helper::BasicStringSingleMatch(L"true", &m_source[0] + m_inx, 4);
		}
		bool IsFalse()
		{
			return Helper::BasicStringSingleMatch(L"false", &m_source[0] + m_inx, 5);
		}
		bool IsSpace(wchar_t &corresponding)
		{
			wchar_t wc = m_source[m_inx];
			if (wc == L'n')
				corresponding = L'\n';
			else if (wc == L'b')
				corresponding = L'\b';
			else if (wc == L'r')
				corresponding = L'\r';
			else if (wc == L't')
				corresponding = L'\t';
			else if (wc == L'f')
				corresponding = L'\f';
			else if (wc == L'"')
				corresponding = L'"';
			else if (wc == L'\\')
				corresponding = L'\\';
			else if (wc == L'/')
				corresponding = L'/';
			else if (wc == L'u')
				corresponding = L'u';
			else
				return false;
			return true;
		}
		bool IsWhite() {
			wchar_t ch = m_source[m_inx];
			return  ch == L' ' || ch == '\n' || ch == '\t' || ch == '\r';
		}
		bool IsHex()
		{
			wchar_t wc = m_source[m_inx];
			return ((wc >= L'0' && wc <= L'9') || (L'a' <= wc && wc <= L'f')
				|| (L'A' <= wc && wc <= L'F'));
		}
		Token ReadString()
		{
			StringBuilder^ target = gcnew StringBuilder();
			for (;;)
			{
				m_inx++;
				if (m_source[m_inx] == L'\\')//转移开头
				{
					m_inx++;
					wchar_t corresponding;
					if (IsSpace(corresponding))//是否为转义符
					{
						if (m_source[m_inx] == L'u')//16进制处理
						{
							for (int i = 0; i < 4; ++i)
							{
								m_inx++;
								if (!IsHex())
									throw gcnew JsonParserException(L"Invalid hex format");
							}
							target->Append((wchar_t)Helper::HexToInt<int>(&m_source[0] + m_inx - 3, 4));//字符处理
						}
						else
						{
							target->Append(corresponding);
						}
					}
					else
						throw gcnew JsonParserException(L"Invalid escape character");
				}
				else if (m_source[m_inx] == L'\r' || m_source[m_inx] == L'\n')
				{
					throw gcnew JsonParserException(L"Invalid character");
				}
				else if (m_source[m_inx] == L'"')
				{
					m_inx++;
					return Token(TokenType::String, target->ToString());
				}
				else
					target->Append(m_source[m_inx]);
			}
		}
		Token ReadNumber()
		{
			size_t count = Helper::FloatSniff(&m_source[0] + m_inx);
			if (count == 0)
				throw gcnew JsonParserException(L"Invalid number format");
			double first_part = Helper::StringToFloat<double>(&m_source[0] + m_inx, count);
			m_inx += count;
			if (m_source[m_inx] == L'E' || m_source[m_inx] == L'e')
			{
				m_inx++;
				size_t sec_count = Helper::FloatSniff(&m_source[0] + m_inx);
				if (sec_count == 0)
					throw gcnew JsonParserException(L"Invalid number format");
				else
				{
					double second_part = Helper::StringToFloat<double>(&m_source[0] + m_inx, sec_count);
					m_inx += sec_count;
					return Token(TokenType::Number, pow(first_part, second_part));
				}
			}
			else
				return Token(TokenType::Number, first_part);
			return Token(TokenType::Number, 0);
		}
	public:
		JsonTokenizer(array<wchar_t>^source):m_source(source),m_size(source->Length) {
		}
		Token Go() {
			while (IsWhite() && m_inx < m_size)
				m_inx++;
			switch (m_source[m_inx])
			{
			case L'{':
				m_inx++;
				return Token(TokenType::ObjectStart, L"{");
			case L'}':
				m_inx++;
				return Token(TokenType::ObjectEnd, L"}");
			case L'[':
				m_inx++;
				return Token(TokenType::ArrayStart, L"[");
			case L']':
				m_inx++;
				return Token(TokenType::ArrayEnd, L"]");
			case L',':
				m_inx++;
				return Token(TokenType::Split, L",");
			case L':':
				m_inx++;
				return Token(TokenType::Represent, L":");
			case L'n':
				if (IsNull())
				{
					m_inx += 4;
					return Token(TokenType::Null, String::Empty);
				}
				else
					throw gcnew JsonParserException(L"Invalid key word : null");
			case L't':
				if (IsTrue())
				{
					m_inx += 4;
					return Token(TokenType::Boolean, 1);
				}
				else
					throw gcnew JsonParserException(L"Invalid key word : true");
			case L'f':
				if (IsFalse())
				{
					m_inx += 5;
					return Token(TokenType::Boolean, 0);
				}
				else
					throw gcnew JsonParserException(L"Invalid key word : false");
			case L'"':
				return ReadString();
			case L'-':
				return ReadNumber();
			}
			if (Helper::IsInt(m_source[m_inx]))
				return ReadNumber();
			throw gcnew JsonParserException(L"Syntax error");
		}
		bool IsDone() {
			return m_inx >= m_size;
		}
	};
	//Json解析器
	public ref class JsonParser
	{
		JsonTokenizer^ m_tokenizer;
		JsonArray^ InnerParseArray(bool skip_self);
	public:
		JsonParser(String^target) {
			m_tokenizer = gcnew JsonTokenizer(target->ToCharArray());
		}
		JsonArray^ ParseArray();
		JsonObject^ ParseObject();
	};
	JsonArray^ JsonParser::InnerParseArray(bool skip_self)
	{
		List<IJsonValue^>^ list = gcnew List<IJsonValue^>();
		if (skip_self)
			m_tokenizer->Go();
		while (!m_tokenizer->IsDone())
		{
			Token current = m_tokenizer->Go();
			switch (current.Type)
			{
			case TokenType::ObjectStart:
				list->Add(ParseObject());
				break;
			case TokenType::ArrayStart:
				list->Add(InnerParseArray(false));
				break;
			case TokenType::String:
				list->Add(gcnew JsonValueInstance(current.Value, 0, JsonValueType::String));
				break;
			case TokenType::Null:
				list->Add(gcnew JsonValueInstance(current.Value, 0, JsonValueType::Null));
				break;
			case TokenType::Number:
				list->Add(gcnew JsonValueInstance(current.Value, current.ValueCache, JsonValueType::Number));
				break;
			case TokenType::Boolean:
				list->Add(gcnew JsonValueInstance(current.Value, 0, JsonValueType::Boolean));
				break;
			case TokenType::Split:
				break;
			case TokenType::ArrayEnd:
				return gcnew JsonArrayInstance(list);
			default:
				throw gcnew JsonParserException(L"Unexpected token");
			}
		}
		throw gcnew JsonParserException(L"Unexpected end");
		return nullptr;
	}
	JsonArray^ JsonParser::ParseArray()
	{
		return InnerParseArray(true);
	}
	JsonObject^ JsonParser::ParseObject()
	{
		Dictionary<String^, IJsonValue^>^ map = gcnew Dictionary<String^, IJsonValue^>();
		bool is_represented = false;
		bool item_is_done = false;
		String^ key = nullptr;
		IJsonValue^ value = nullptr;
		while (!m_tokenizer->IsDone())
		{
			Token current = m_tokenizer->Go();
			switch (current.Type)
			{
			case TokenType::Represent:
				is_represented = true;
				break;
			case TokenType::ObjectStart:
				if (is_represented)
				{
					value = ParseObject();
					is_represented = false;
					item_is_done = true;
				}
				break;
			case TokenType::String:
				if (is_represented)
				{
					value = gcnew JsonValueInstance(current.Value, 0, JsonValueType::String);
					is_represented = false;
					item_is_done = true;
				}
				else
					key = current.Value;
				break;
			case TokenType::Number:
				if (is_represented)
				{
					value = gcnew JsonValueInstance(current.Value, current.ValueCache, JsonValueType::Number);
					is_represented = false;
					item_is_done = true;
				}
				else
					throw gcnew JsonParserException(L"Numbers can only be values");
				break;
			case TokenType::Boolean:
				if (is_represented)
				{
					value = gcnew JsonValueInstance(current.Value, 0, JsonValueType::Boolean);
					is_represented = false;
					item_is_done = true;
				}
				else
					throw gcnew JsonParserException(L"Booleans can only be values");
				break;
			case TokenType::ArrayStart:
				if (is_represented)
				{
					value = InnerParseArray(false);
					is_represented = false;
					item_is_done = true;
					break;
				}
				else
					throw gcnew JsonParserException(L"Array can only be values");
			case TokenType::Null:
				if (is_represented)
				{
					value = gcnew JsonValueInstance(String::Empty, 0, JsonValueType::Null);
					is_represented = false;
					item_is_done = true;
					break;
				}
				else
					throw gcnew JsonParserException(L"Array can only be values");
			case TokenType::Split:
				break;
			case TokenType::ObjectEnd:
				return gcnew JsonObjectInstance(map);
			}
			if (item_is_done)
			{
				map->Add(key, value);
				item_is_done = false;
			}
		}
		throw gcnew JsonParserException(L"Syntax error");
		return nullptr;
	}

	public enum class FieldFlag
	{
		PrimitiveType,
		List,
		Object
	};

	[AttributeUsageAttribute(AttributeTargets::Class|AttributeTargets::Struct)]
	public ref class JsonObjectificationAttribute :Attribute
	{

	};

	[AttributeUsageAttribute(AttributeTargets::Property)]
	public ref class JsonFieldAttribute :Attribute
	{
		String^ m_target; 
		FieldFlag m_flag;
	public:
		property String^ Target{
			String^ get() {
				return m_target;
			}
			void set(String^ in) {
				m_target = in;
			}
		}
		property FieldFlag Flag {
			FieldFlag get() {
				return m_flag;
			}
			void set(FieldFlag in) {
				m_flag = in;
			}
		}
	};

	private ref class Setter
	{
	public:
		FieldFlag Flag;
		PropertyInfo^ Target;
		Type^ SecondaryType;
		String^ JsonKey;
		List<Setter^>^ ChildSetters;
	};

	generic<class T> where T:ref class
	public ref class JsonObjectification
	{
		List<Setter^>^ m_setters;
		//IList解析
		void CreateArray(Type^ GenericType, Setter^ Set, bool TypeCheck) {
			if (Set == nullptr)
				return;
			if (TypeCheck)//是否需要检查类型
			{
				if (!GenericType->IsGenericType)//不是泛型的话
					return;
				if (!(GenericType->GetGenericTypeDefinition() == IList::typeid))//IList限定
					return;
			}
			array<Type^>^ types = GenericType->GetGenericArguments();
			if (TypeCheck)//是否需要检查类型
				if (types == nullptr || types->Length != 1)//仅有一个参数
					return;
			Type^ inner = types[0];
			if (inner->IsGenericType)//泛型
				if (inner->GetGenericTypeDefinition() == IList::typeid)//List
				{
					Set->SecondaryType = inner;//记录嵌套类型
					Setter^ setter = gcnew Setter();//保留Setter
					setter->Flag = FieldFlag::List;//表明为List(CreateArray可能会改变这个Flag，取决于是否是最后一层
					CreateArray(inner, setter, false);
					Set->ChildSetters = gcnew List<Setter^>(1);
					Set->ChildSetters->Add(setter);//保留层级信息
					return;
				}
			if (inner->IsClass&&inner != String::typeid)//UDT
				Set->ChildSetters = CreateObject(inner);
			else
				Set->Flag = FieldFlag::PrimitiveType;
			Set->SecondaryType = inner;
		}
		//Object解析
		List<Setter^>^ CreateObject(Type^ Target)
		{
			if (Target == nullptr || Attribute::GetCustomAttribute(Target, JsonObjectificationAttribute::typeid) == nullptr)//未标记Attribute
				return nullptr;
			List<Setter^>^ ret = gcnew List<Setter^>();
			for each(PropertyInfo^% info in Target->GetProperties())
			{
				JsonFieldAttribute^attr = safe_cast<JsonFieldAttribute^>(Attribute::GetCustomAttribute(info, JsonFieldAttribute::typeid));
				if (attr == nullptr)//有标识的Attribute
					continue;
				if (!info->CanWrite)//保证可写
					continue;
				Setter^ setter = gcnew Setter();
				switch (attr->Flag)
				{
				case FieldFlag::Object:
					setter->ChildSetters = CreateObject(info->PropertyType); break;
				case FieldFlag::List:
					CreateArray(info->PropertyType, setter, true); break;
				}
				setter->Target = info;
				setter->JsonKey = attr->Target;
				setter->Flag = attr->Flag;
				ret->Add(setter);
			}
			return ret;
		}
		//设置对象
		void SetObject(JsonObject^ Target, Object^% Dest, List<Setter^>^ Setters) {
			if (Target == nullptr || Dest == nullptr || Setters == nullptr)
				return;
			for each (Setter^% var in Setters)
			{
				if (var->Flag == FieldFlag::PrimitiveType)//基元类型
				{
					JsonValue^ value = Target->GetValue(var->JsonKey);
					if (value != nullptr)
						var->Target->SetValue(Dest, value->GetBoxedValue());
				}
				else if (var->Flag == FieldFlag::Object)//对象
				{
					JsonObject^ json_object = Target->GetObject(var->JsonKey);
					if (json_object != nullptr)
					{
						Object^ subobject = Activator::CreateInstance(var->Target->PropertyType);
						SetObject(json_object, subobject, var->ChildSetters);
						var->Target->SetValue(Dest, subobject);
					}
				}
				else if (var->Flag == FieldFlag::List)//数组
				{
					JsonArray^ json_array = Target->GetArray(var->JsonKey);
					if (json_array != nullptr)
					{
						System::Collections::IList^ list = (System::Collections::IList^)Activator::CreateInstance(List::typeid->MakeGenericType(var->SecondaryType), (Object^)json_array->Count);
						SetArray(json_array, list, var);
						var->Target->SetValue(Dest, list);
					}
				}
			}
		}
		//设置Array
		void SetArray(JsonArray^ Target, System::Collections::IList^ Dest, Setter^ Set)
		{
			if (Set->Flag == FieldFlag::List)//(嵌套类型)
			{
				for each(IJsonValue^% item in Target)
				{
					if (item->GetValueType() != JsonValueType::Array)
						continue;
					System::Collections::IList^ sublist = (System::Collections::IList^)Activator::CreateInstance(List::typeid->MakeGenericType(Set->SecondaryType->GenericTypeArguments[0]));
					SetArray(safe_cast<JsonArray^>(item), sublist, Set->ChildSetters[0]);
					Dest->Add(sublist);
				}
			}
			else 
			{
				for each(IJsonValue^% item in Target)
				{
					Object^subobject = nullptr;
					if (Set->SecondaryType->IsClass&&Set->SecondaryType != String::typeid)
					{
						if (item->GetValueType() == JsonValueType::Object)
						{
							subobject = Activator::CreateInstance(Set->SecondaryType);
							SetObject(safe_cast<JsonObject^>(item), subobject, Set->ChildSetters);
						}
					}
					else
					{
						if (IJsonValue::IsValue(item))
							subobject = safe_cast<JsonValue^>(item)->GetBoxedValue();
					}
					if (subobject != nullptr)
						Dest->Add(subobject);
				}
			}
		}
	public:
		JsonObjectification() {
			m_setters = CreateObject(T::typeid);
		}
		T Go(JsonObject^ Target)
		{
			Object^ ret = System::Activator::CreateInstance(T::typeid);
			SetObject(Target, ret, m_setters);
			return (T)ret;
		}
	};
}
