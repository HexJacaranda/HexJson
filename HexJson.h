#pragma once
#include<vcclr.h>
#include<cmath>

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;

namespace HexJson {
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
	public interface class IJsonValue
	{
	public:
		virtual property JsonValueType ValueType { JsonValueType get(); };
	};

	[System::SerializableAttribute]
	public ref class JsonRuntimeException : Exception
	{
	public:
		JsonRuntimeException() { }
		JsonRuntimeException(String^ message) : Exception(message) { }
		JsonRuntimeException(String^ message, Exception^ inner) : Exception(message, inner) { }
	protected:
		JsonRuntimeException(
			Runtime::Serialization::SerializationInfo^ info,
			Runtime::Serialization::StreamingContext context) : Exception(info, context) { }
	};

	[System::SerializableAttribute]
	public ref class JsonParsingException : Exception
	{
	public:
		JsonParsingException() { }
		JsonParsingException(String^ message) : Exception(message) { }
		JsonParsingException(String^ message, Exception^ inner) : Exception(message, inner) { }
	protected:
		JsonParsingException(
			System::Runtime::Serialization::SerializationInfo^ info,
			System::Runtime::Serialization::StreamingContext context) : Exception(info, context) { }
	};

	public ref class JsonValue : IJsonValue
	{
	protected:
		String^ m_string;
		double m_cache;
		JsonValueType m_type;
	public:
		JsonValue(String^ value, double cache, JsonValueType type)
		{
			m_cache = cache;
			m_string = value;
			m_type = type;
		}
		virtual property JsonValueType ValueType
		{
			JsonValueType get() {
				return m_type;
			}
		}
		double AsDouble()
		{
			if (m_type == JsonValueType::Number)
				return m_cache;
			throw gcnew JsonRuntimeException("Not float");
		}
		String^ AsString()
		{
			if (m_type == JsonValueType::String)
				return m_string;
			throw gcnew JsonRuntimeException("Not string");
		}
		int AsInt()
		{
			if (m_type == JsonValueType::Number)
				return (int)m_cache;
			throw gcnew JsonRuntimeException("Not float");
		}
		bool AsBoolean()
		{
			if (m_type == JsonValueType::Boolean)
				return m_cache == 1;
			throw gcnew JsonRuntimeException("Not boolean");
		}
		Object^ GetValue()
		{
			if (m_type == JsonValueType::String)
				return m_string;
			else if (m_type == JsonValueType::Boolean)
				return m_cache == 0 ? false : true;
			else if (m_type == JsonValueType::Null)
				return nullptr;
			else
				return m_cache;
			
		}
		static bool IsValue(IJsonValue^ value)
		{
			JsonValueType type = value->ValueType;
			return (type == JsonValueType::Boolean || type == JsonValueType::Number || type == JsonValueType::Null || type == JsonValueType::String);
		}
		static JsonValue^ From(String^ value)
		{
			return gcnew JsonValue(value, 0, JsonValueType::String);
		}
		static JsonValue^ From(double value)
		{
			return gcnew JsonValue(nullptr, value, JsonValueType::Number);
		}
		static JsonValue^ From(bool value)
		{
			return gcnew JsonValue(nullptr, value ? 1 : 0, JsonValueType::Boolean);
		}
		static JsonValue^ From()
		{
			return gcnew JsonValue(nullptr, 0, JsonValueType::Null);
		}
	};
	public ref class JsonObject : IJsonValue, IEnumerable<KeyValuePair<String^, IJsonValue^>>
	{
	protected:
		Dictionary<String^, IJsonValue^>^ m_map;
		generic<class T>
			T TryGetAs(String^ index, JsonValueType type)
			{
				IJsonValue^ result = nullptr;
				bool found = m_map->TryGetValue(index, result);
				if (!found)
					return T();
				if (result->ValueType == type)
					return (T)result;
				return T();
			}
	public:
		JsonObject(Dictionary<String^, IJsonValue^>^ target)
		{
			m_map = target;
		}
		JsonObject()
		{
			m_map = gcnew Dictionary<String^, IJsonValue^>();
		}
		virtual property JsonValueType ValueType
		{
			JsonValueType get() {
				return JsonValueType::Object;
			}
		}
		property IJsonValue^ default[String^]
		{
			IJsonValue^ get(String^ index)
			{
				return m_map[index];
			}
		}
		property int Count
		{
			int get()
			{
				return m_map->Count;
			}
		}
		JsonValue^ GetValue(String^ index)
		{
			IJsonValue^ result = nullptr;
			bool found = m_map->TryGetValue(index, result);
			if (!found)
				return nullptr;
			JsonValueType type = result->ValueType;
			if (type == JsonValueType::Boolean ||
				type == JsonValueType::Number ||
				type == JsonValueType::Null ||
				type == JsonValueType::String)
				return (JsonValue^)result;
			else
				return nullptr;
		}
		JsonObject^ GetObject(String^ index)
		{
			return TryGetAs<JsonObject^>(index, JsonValueType::Object);
		}
		JsonArray^ GetArray(String^ index)
		{
			return TryGetAs<JsonArray^>(index, JsonValueType::Array);
		}
		void AddItem(String^ key, IJsonValue^ value)
		{
			m_map->Add(key, value);
		}
		void RemoveItem(String^ key)
		{
			m_map->Remove(key);
		}
		virtual IEnumerator<KeyValuePair<String^, IJsonValue^>>^ GetEnumerator()
		{
			return m_map->GetEnumerator();
		}
	protected:
		virtual Collections::IEnumerator^ GetBoxedEnumerator() = Collections::IEnumerable::GetEnumerator
		{
			return m_map->GetEnumerator();
		}

	};
	public ref class JsonArray : IJsonValue, IEnumerable<IJsonValue^>
	{
	protected:
		List<IJsonValue^>^ m_list;
	public:
		JsonArray(List<IJsonValue^>^ target)
		{
			m_list = target;
		}
		JsonArray()
		{
			m_list = gcnew List<IJsonValue^>();
		}
		virtual property JsonValueType ValueType
		{
			JsonValueType get() {
				return JsonValueType::Array;
			}
		}
		property IJsonValue^ default[int]
		{
			IJsonValue ^ get(int index)
			{
				return m_list[index];
			}
		}
		property int Count
		{
			int get()
			{
				return m_list->Count;
			}
		}
		JsonObject^ GetObject(int index)
		{
			IJsonValue^ value = m_list[index];
			return value->ValueType == JsonValueType::Object ? (JsonObject^)value : nullptr;
		}
		JsonValue^ GetValue(int index)
		{
			IJsonValue^ value = m_list[index];
			return JsonValue::IsValue(value) ? (JsonValue^)value : nullptr;
		}
		JsonArray^ GetArray(int index)
		{
			IJsonValue^ value = m_list[index];
			return value->ValueType == JsonValueType::Array ? (JsonArray^)value : nullptr;
		}
		void AddItem(IJsonValue^ value)
		{
			m_list->Add(value);
		}
		virtual IEnumerator<IJsonValue^>^ GetEnumerator()
		{
			return m_list->GetEnumerator();
		}
	protected:
		virtual Collections::IEnumerator^ GetBoxedEnumerator()=Collections::IEnumerable::GetEnumerator
		{
			return m_list->GetEnumerator();
		}
	};
	class JsonParseHelper
	{
	public:
		template<class CharT>
		static bool BasicStringSingleMatch(const CharT* target_string, const CharT* source, size_t target_length) {
			if (target_string[0] == source[0] && target_string[target_length - 1] == source[target_length - 1]) {
				for (size_t i = 1; i < target_length - 1; ++i) {
					if (target_string[i] != source[i])
						return false;
				}
				return true;
			}
			return false;
		}
		static int FloatSniff(interior_ptr<const wchar_t> value, int index)
		{

			int ret = 0;
			short dot_times = 0;
			if (value[index] == '-' || Char::IsDigit(value[index]))
			{
				index++;
				ret++;
				while (true)
				{
					if (value[index] == '.')
					{
						++dot_times;
						if (dot_times > 1)
							return 0;
					}
					else if (!Char::IsDigit(value[index]))
						break;
					++index;
					++ret;
				}
			}
			return ret;
		}
		static bool IsHex(wchar_t c)
		{
			return c >= 'a' && c <= 'f';
		}
		static wchar_t HexToChar(interior_ptr<const wchar_t> value, int index, int count)
		{
			int ret = 0;
			int factor = 0;
			for (int i = count - 1; i >= 0; --i)
			{
				if (IsHex(value[index]))
					factor = (value[index] - 'a') + 10;
				else if (Char::IsDigit(value[index]))
					factor = value[index] - '0';
				else
					return (wchar_t)ret;
				ret += factor * (int)std::pow(16, i);
				index++;
			}
			return (wchar_t)ret;
		}
		template<class FloatT, class CharT>
		static FloatT StringToFloat(const CharT* string, size_t count) {
			bool sign = string[0] == (CharT)'-';
			bool dot_sign = false;
			FloatT result = (FloatT)0;
			FloatT floater = (FloatT)0;
			size_t index = 0;
			for (; index < count; ++index) {
				result *= 10;
				if (string[index] == L'.') {
					index++;
					break;
				}
				result += string[index] - (CharT)'0';
			}
			for (size_t second = count - 1; second >= index; --second) {
				floater += string[second] - (CharT)'0';
				floater /= 10;
			}
			result += floater;
			return sign ? -result : result;
		}
	};
	enum class JsonTokenType
	{
		String,
		Digit,
		Null,
		Boolean,
		LBracket,
		RBracket,
		LCurly,
		RCurly,
		Comma,
		Colon
	};
	ref struct JsonToken
	{
	public:
		double Value;
		String^ Content;
		JsonTokenType Type;
		JsonToken() {
			Content = nullptr;
			Value = 0.0;
			Type = JsonTokenType::Null;
		}
		JsonToken(JsonTokenType type, String^ value)
		{
			Type = type;
			Content = value;
			Value = 0;
		}
		JsonToken(JsonTokenType type, double value)
		{
			Type = type;
			Value = value;
			Content = String::Empty;
		}
	};
	ref struct JsonTokenizer
	{
		const wchar_t* m_source;
		int m_index;
		int m_length;
		bool m_end;
		void SetSingleToken(JsonToken% token, JsonTokenType type)
		{
			token.Type = type;
			token.Value = m_source[m_index];
			if (m_index == m_length - 1)
			{
				m_end = true;
				return;
			}
			m_index++;
		}
		static bool GetEscapeChar(wchar_t wc, wchar_t% corresponding)
		{
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
		void ReadString(JsonToken% token)
		{
			StringBuilder^ builder = gcnew StringBuilder(16);
			token.Type = JsonTokenType::String;
			m_index++;
			for (; ; )
			{
				if (m_source[m_index] == L'\\')//转义
				{
					m_index++;
					if (m_source[m_index] == L'u')//Unicode转义
					{
						m_index++;
						wchar_t unicode = JsonParseHelper::HexToChar(m_source, m_index, 4);
						builder->Append(unicode);
						m_index += 4;
					}
					else
					{
						wchar_t escape = Char::MinValue;
						if (!GetEscapeChar(m_source[m_index], escape))
							throw gcnew JsonParsingException(L"Invalid escape character");
						builder->Append(escape);
						m_index++;
					}
				}
				else if (m_source[m_index] == L'"')
				{
					token.Content = builder->ToString();
					m_index++;
					return;
				}
				else
					builder->Append(m_source[m_index++]);
			}
		}
		void ReadDigit(JsonToken% token)
		{
			int count = JsonParseHelper::FloatSniff(m_source, m_index);
			if (count == 0)
				throw gcnew JsonParsingException(L"Nought-length number is not allowed");
			double first_part = JsonParseHelper::StringToFloat<double>(m_source + m_index, count);
			m_index += count;
			if (m_source[m_index] == L'E' || m_source[m_index] == L'e')
			{
				m_index++;
				int sec_count = JsonParseHelper::FloatSniff(m_source, m_index);
				if (sec_count == 0)
					throw gcnew JsonParsingException(L"Nought-length exponent is not allowed");
				else
				{
					double second_part = JsonParseHelper::StringToFloat<double>(m_source + m_index, sec_count);
					m_index += sec_count;
					token.Value = std::pow(first_part, second_part);
				}
			}
			else
				token.Value = first_part;
			token.Type = JsonTokenType::Digit;
		}
		void ReadNull(JsonToken% token)
		{
			token.Type = JsonTokenType::Null;
			if (!JsonParseHelper::BasicStringSingleMatch(L"null", m_source + m_index, 4))
				throw gcnew JsonParsingException(L"Invalid key word - null");
			token.Content = L"null";
			m_index += 4;
		}
		void ReadTrue(JsonToken% token)
		{
			token.Type = JsonTokenType::Boolean;
			token.Value = 1;			
			if (!JsonParseHelper::BasicStringSingleMatch(L"true", m_source + m_index, 4))
				throw gcnew JsonParsingException(L"Invalid boolean value");
			token.Content = L"true";
			m_index += 4;
		}
		void ReadFalse(JsonToken% token)
		{
			token.Type = JsonTokenType::Boolean;
			token.Value = 0;			
			if (!JsonParseHelper::BasicStringSingleMatch(L"false", m_source + m_index, 5))
				throw gcnew JsonParsingException(L"Invalid boolean value");
			token.Content = L"false";
			m_index += 5;
		}
	public:
		JsonTokenizer(wchar_t const* JsonString, int Length)
		{
			m_source = JsonString;
			m_length = Length;
			m_index = 0;
			m_end = false;
		}
		void Consume(JsonToken% token)
		{
			while (Char::IsWhiteSpace(m_source[m_index])) m_index++;
			wchar_t current = m_source[m_index];
			switch (current)
			{
			case L'{':
				SetSingleToken(token, JsonTokenType::LCurly); break;
			case L'}':
				SetSingleToken(token, JsonTokenType::RCurly); break;
			case L'[':
				SetSingleToken(token, JsonTokenType::LBracket); break;
			case L']':
				SetSingleToken(token, JsonTokenType::RBracket); break;
			case L',':
				SetSingleToken(token, JsonTokenType::Comma); break;
			case L':':
				SetSingleToken(token, JsonTokenType::Colon); break;
			case L'"':
				ReadString(token); break;
			case L'-':
				ReadDigit(token); break;
			case L'n':
				ReadNull(token); break;
			case L't':
				ReadTrue(token); break;
			case L'f':
				ReadFalse(token); break;
			default:
				if (Char::IsDigit(current))
					ReadDigit(token);
				break;
			}
		}
		property bool Done {
			bool get() {
				return m_index >= m_length;
			}
		}
		void Repeek(int Cnt)
		{
			m_index -= Cnt;
		}
	};
	class JsonParser
	{		
	public:
		static IJsonValue^ ParseValue(JsonTokenizer% tokenizer)
		{
			JsonToken token;
			if (tokenizer.Done)
				return nullptr;
			tokenizer.Consume(token);
			switch (token.Type)
			{
			case JsonTokenType::LCurly:
				tokenizer.Repeek(1);
				return ParseObject(tokenizer);
			case JsonTokenType::LBracket:
				tokenizer.Repeek(1);
				return ParseArray(tokenizer);
			case JsonTokenType::String:
				return gcnew JsonValue(token.Content, token.Value, JsonValueType::String);
			case JsonTokenType::Digit:
				return gcnew JsonValue(token.Content, token.Value, JsonValueType::Number);
			case JsonTokenType::Boolean:
				return gcnew JsonValue(token.Content, token.Value, JsonValueType::Boolean);
			case JsonTokenType::Null:
				return gcnew JsonValue(token.Content, token.Value, JsonValueType::Null);
			}
			return nullptr;
		}
	    static JsonObject^ ParseObject(JsonTokenizer% tokenizer)
		{
			Dictionary<String^, IJsonValue^>^ table = gcnew Dictionary<String^, IJsonValue^>();
			JsonToken token;
			tokenizer.Consume(token);
			if (token.Type != JsonTokenType::LCurly)
				throw gcnew JsonParsingException(L"Expected to be LCurly({)");
			while (!tokenizer.Done)
			{
				tokenizer.Consume(token);
				if (token.Type != JsonTokenType::String)
				{
					if (token.Type == JsonTokenType::RCurly)
						break;
					throw gcnew JsonParsingException(L"Expected to be String");
				}
				String^ key = token.Content;
				tokenizer.Consume(token);
				if (token.Type != JsonTokenType::Colon)
					throw gcnew JsonParsingException(L"Expected to be Colon(:)");
				IJsonValue^ value = ParseValue(tokenizer);
				table->Add(key, value);
				tokenizer.Consume(token);
				if (token.Type == JsonTokenType::RCurly)
					break;
				if (token.Type != JsonTokenType::Comma)
					throw gcnew JsonParsingException(L"Expected to be Comma(,)");
			}
			return gcnew JsonObject(table);
		}
		static JsonArray^ ParseArray(JsonTokenizer% tokenizer)
		{
			List<IJsonValue^>^ list = gcnew List<IJsonValue^>();
			JsonToken token;
			tokenizer.Consume(token);
			if (token.Type != JsonTokenType::LBracket)
				throw gcnew JsonParsingException(L"Expected to be LBracket([)");
			while (!tokenizer.Done)
			{
				IJsonValue^ value = ParseValue(tokenizer);
				if (value == nullptr)
					break;
				list->Add(value);
				tokenizer.Consume(token);
				if (token.Type == JsonTokenType::RBracket)
					break;
				if (token.Type != JsonTokenType::Comma)
					throw gcnew JsonParsingException(L"Expected to be Comma(,)");
			}
			return gcnew JsonArray(list);
		}
	};
	public ref class Json
	{
	public:
		static IJsonValue^ Parse(String^ target) {
			pin_ptr<const wchar_t> ptr = PtrToStringChars(target);
			JsonTokenizer tokenizer(ptr, target->Length);
			return target->StartsWith(L"{") ? (IJsonValue^)JsonParser::ParseObject(tokenizer) : (IJsonValue^)JsonParser::ParseArray(tokenizer);
		}
		static JsonObject^ ParseObject(String^ target) {
			pin_ptr<const wchar_t> ptr = PtrToStringChars(target);
			JsonTokenizer tokenizer(ptr, target->Length);
			return JsonParser::ParseObject(tokenizer);
		}
		static JsonArray^ ParseArray(String^ target) {
			pin_ptr<const wchar_t> ptr = PtrToStringChars(target);
			JsonTokenizer tokenizer(ptr, target->Length);
			return JsonParser::ParseArray(tokenizer);
		}
	};

	[System::AttributeUsageAttribute(AttributeTargets::Class | AttributeTargets::Struct)]
	public ref class JsonDeserializationAttribute : Attribute { };

	[System::AttributeUsageAttribute(AttributeTargets::Property | AttributeTargets::Field)]
	public ref class JsonFieldAttribute : Attribute
	{
	protected:
		String^ m_field;
	public:
		JsonFieldAttribute(String^ Field) :m_field(Field) {}
		property String^ JsonField {
			String^ get() {
				return m_field;
			}
			void set(String^ value) {
				m_field = value;
			}
		}
	};

	public enum class FieldType
	{
		Property,
		Field,
		List,
		ListWithNest
	};

	public ref class FieldSetter
	{
	public:
		FieldType SetterFieldType;
		PropertyInfo^ Property;
		Type^ FieldType;
		FieldInfo^ Field;
		Type^ NestedType;
		String^ JsonKey;
		array<FieldSetter^>^ ChildSetters;
	};

	public ref class JsonDeserializationMetaData
	{
	public:
		Type^ ObjectType;
		array<FieldSetter^>^ Setters;
		JsonDeserializationMetaData(Type^ target, array<FieldSetter^>^ setters)
			:ObjectType(target), Setters(setters) {}
	};

	public ref class JsonDeserialization
	{
	private:
		static array<FieldSetter^>^ Parse(Type^ ObjectType)
		{
			auto setters = gcnew List<FieldSetter^>();
			if (ObjectType == nullptr || !ObjectType->IsDefined(JsonDeserializationAttribute::typeid, false))
				return nullptr;
			for each (auto % member in ObjectType->GetMembers())
			{
				JsonFieldAttribute^ attribute = nullptr;
				if (member->MemberType == MemberTypes::Property || member->MemberType == MemberTypes::Field)
				{
					attribute = safe_cast<JsonFieldAttribute^>(member->GetCustomAttributes(JsonFieldAttribute::typeid, false)[0]);
					if (attribute == nullptr)
						continue;
				}
				else
					continue;
				FieldSetter^ setter = gcnew FieldSetter();
				setter->JsonKey = attribute->JsonField;
				if (member->MemberType == MemberTypes::Property)
				{
					setter->SetterFieldType = FieldType::Property;
					auto property_info = safe_cast<PropertyInfo^>(member);
					if (!property_info->CanWrite)
						continue;
					setter->Property = property_info;
					setter->FieldType = property_info->PropertyType;
				}
				else if (member->MemberType == MemberTypes::Field)
				{
					setter->SetterFieldType = FieldType::Field;
					setter->Field = safe_cast<FieldInfo^>(member);
					setter->FieldType = setter->Field->FieldType;
				}
				if (setter->FieldType->IsClass && setter->FieldType != String::typeid && setter->FieldType != Object::typeid)//非基本类型
				{
					if (setter->FieldType->IsArray)//数组
					{
						setter->NestedType = setter->FieldType->GetElementType();
						if (setter->NestedType != String::typeid && setter->NestedType != Object::typeid && !setter->NestedType->IsPrimitive)
						{
							setter->SetterFieldType = FieldType::ListWithNest;
							setter->ChildSetters = Parse(setter->NestedType);
						}
						else
							setter->SetterFieldType = FieldType::List;
					}
					else if (setter->FieldType->IsConstructedGenericType)//动态数组
					{
						if ((Collections::IList::typeid)->IsAssignableFrom(setter->FieldType))
						{
							setter->NestedType = setter->FieldType->GetGenericArguments()[0];
							if (setter->NestedType != String::typeid && setter->NestedType != Object::typeid && !setter->NestedType->IsPrimitive)
							{
								setter->SetterFieldType = FieldType::ListWithNest;
								setter->ChildSetters = Parse(setter->NestedType);
							}
							else
								setter->SetterFieldType = FieldType::List;
						}
						else
							continue;
					}
					else
						setter->ChildSetters = Parse(setter->FieldType);
				}
				setters->Add(setter);
			}
			return setters->ToArray();
		}
		static Object^ DeserializeObject(Type^ TargetType, JsonObject^ JsonTarget, array<FieldSetter^>^ Setters)
		{
			Object^ ret = Activator::CreateInstance(TargetType);
			for each (auto % setter in Setters)
			{
				switch (setter->SetterFieldType)
				{
				case FieldType::Field:
					setter->Field->SetValue(ret, Convert::ChangeType(JsonTarget->GetValue(setter->JsonKey)->GetValue(), setter->FieldType));
					break;
				case FieldType::Property:
					setter->Property->SetValue(ret, Convert::ChangeType(JsonTarget->GetValue(setter->JsonKey)->GetValue(), setter->FieldType));
					break;
				case FieldType::List:
				{
					JsonArray^ json_array = JsonTarget->GetArray(setter->JsonKey);
					Object^ value = nullptr;
					if (setter->FieldType->IsArray)
					{
						Array^ array = safe_cast<Array^>(Activator::CreateInstance(setter->FieldType, json_array->Count));
						value = array;
						for (int i = 0; i < array->Length; ++i)
							array->SetValue(Convert::ChangeType(json_array->GetValue(i)->GetValue(), setter->NestedType), i);
					}
					else
					{
						Collections::IList^ list = safe_cast<Collections::IList^>(Activator::CreateInstance(setter->FieldType));
						value = list;
						for each (auto % item in json_array)
							list->Add(Convert::ChangeType(safe_cast<JsonValue^>(item)->GetValue(), setter->NestedType));
					}
					if (setter->Field != nullptr)
						setter->Field->SetValue(ret, value);
					else
						setter->Property->SetValue(ret, value);
					break;
				}
				case FieldType::ListWithNest:
				{
					JsonArray^ json_array = JsonTarget->GetArray(setter->JsonKey);
					Object^ value = nullptr;
					if (setter->FieldType->IsArray)
					{
						Array^ array = safe_cast<Array^>(Activator::CreateInstance(setter->FieldType, json_array->Count));
						value = array;
						for (int i = 0; i < array->Length; ++i)
							array->SetValue(DeserializeObject(setter->NestedType, json_array->GetObject(i), setter->ChildSetters), i);
					}
					else
					{
						Collections::IList^ list = safe_cast<Collections::IList^>(Activator::CreateInstance(setter->FieldType));
						value = list;
						for each (auto % item in json_array)
							list->Add(DeserializeObject(setter->NestedType, safe_cast<JsonObject^>(item), setter->ChildSetters));
					}
					if (setter->Field != nullptr)
						setter->Field->SetValue(ret, value);
					else
						setter->Property->SetValue(ret, value);
				}
				break;
				}
			}
			return ret;
		}
	public:
		generic<class T>
		static JsonDeserializationMetaData^ GetMetaData()
		{
			return gcnew JsonDeserializationMetaData(T::typeid, Parse(T::typeid));
		}
		static JsonDeserializationMetaData^ GetMetaData(Type^ TargetType)
		{
			return gcnew JsonDeserializationMetaData(TargetType, Parse(TargetType));
		}
		static Object^ Deserialize(JsonDeserializationMetaData^ MetaData, JsonObject^ JsonTarget)
		{
			return DeserializeObject(MetaData->ObjectType, JsonTarget, MetaData->Setters);
		}
	};
}
