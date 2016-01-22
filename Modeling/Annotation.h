/**
* @addtogroup Modeling Modeling Support Utilities
* @file
* @brief Annotations
* @author Zhenyu Wu
* @date Jan 30, 2015: Refactored from Identifier
**/

#ifndef Annotation_H
#define Annotation_H

#include "BaseLib/Exception.h"
#include "BaseLib/DebugLog.h"
#include "BaseLib/ManagedRef.h"

//======== Annotation ========
template<typename TNote>
class IAnnotation {
	friend SimpleAllocator < IAnnotation > ;
public:
	TNote const Value;

	IAnnotation(TNote const& xValue) : Value(xValue) {}
	IAnnotation(TNote && xValue) : Value(std::move(xValue)) {}
	virtual ~IAnnotation(void) {}

	virtual TString NoteString(void)
	{ return TStringCast(Value); }

	virtual TString toString(void)
	{ return TStringCast(_T('{') << NoteString() << _T('}')); }
};

//======== Annotated ========
template<typename TValue, typename TNote>
class IAnnotated {
public:
	TValue const Value;
	ManagedRef<IAnnotation<TNote>> const rNote;

	IAnnotated(TValue const& xValue) : Value(xValue) {}
	IAnnotated(TValue && xValue) : Value(std::move(xValue)) {}
	IAnnotated(TValue const& xValue, TNote const& xNote) :
		Value(xValue), rNote(EMPLACE_CONSTRUCT, xNote) {}
	IAnnotated(TValue const& xValue, TNote && xNote) :
		Value(xValue), rNote(EMPLACE_CONSTRUCT, std::move(xNote)) {}
	IAnnotated(TValue && xValue, TNote const& xNote) :
		Value(std::move(xValue)), rNote(EMPLACE_CONSTRUCT, xNote) {}
	IAnnotated(TValue && xValue, TNote && xNote) :
		Value(std::move(xValue)), rNote(EMPLACE_CONSTRUCT, std::move(xNote)) {}
	virtual ~IAnnotated(void) {}

	virtual TString ValueString(void)
	{ return TStringCast(Value); }

	virtual TString toString(void)
	{ return TStringCast(ValueString() << rNote.Empty() ? rNote->toString() : _T('')); }
};

//======== AnnotatedObj ========
template<class TObject, typename TNote>
class IAnnotatedObj : public TObject {
private:
	template<class T>
	struct Has_toString {
		template<class C> static int  probe(decltype(&C::toString));
		template<class C> static char probe(...);
		static const bool value = sizeof(probe<T>(nullptr)) > 1;
	};

	template<typename X = TObject>
	auto _toString(void) const -> decltype(std::enable_if<Has_toString<X>::value, TString>::type())
	{ return TStringCast(TObject::toString() << rNote.Empty() ? rNote->toString() : _T('')); }

	template<typename X = TObject, typename = void>
	auto _toString(void) const -> decltype(std::enable_if<!Has_toString<X>::value, TString>::type())
	{ return TStringCast(_T("AnObj@") << (void*)this << rNote.Empty() ? rNote->toString() : _T('')); }

public:
	ManagedRef<IAnnotation<TNote>> const rNote;

	template<typename A, typename... Params,
		typename = std::enable_if<!std::is_same<TNote const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<TNote &&, A>::value>::type
	>
	IAnnotatedObj(A &&xA, Params&&... xParams) : TObject(xA, xParams...) {}

	template<typename A, typename... Params,
		typename = std::enable_if<std::is_same<TNote const&, A>::value>::type,
		typename = std::enable_if<!std::is_same<TNote &&, A>::value>::type,
		typename = void
	>
	IAnnotatedObj(A &&xA, Params&&... xParams) :
	TObject(xParams...), rNote(EMPLACE_CONSTRUCT, xA) {}

	template<typename A, typename... Params,
		typename = std::enable_if<std::is_same<TNote &&, A>::value>::type
	>
	IAnnotatedObj(A &&xA, Params&&... xParams) :
	TObject(xParams...), rNote(EMPLACE_CONSTRUCT, std::move(xA)) {}

	// Copy construction
	IAnnotatedObj(IAnnotatedObj const& xObj) : TObject(xObj), rNote(xObj.rNote) {}
	// Move construction
	IAnnotatedObj(IAnnotatedObj&&) : TObject(std::move(xObj)), rNote(std::move(xObj.rNote)) {}

	~IAnnotatedObj(void) override {}

	template<typename... Params>
	inline static TObject* Create(Params&&... xParams)
	{ return new IAnnotatedObj(xParams...); }

	IAnnotatedObj& operator=(IAnnotatedObj const& xObj) {
		TObject::operator=(xObj);
		*const_cast<ManagedRef<IAnnotation<TNote>>*>(&rNote) = xObj.rNote;
		return *this;
	}
	IAnnotatedObj& operator=(IAnnotatedObj &&xObj) {
		TObject::operator=(std::move(xObj));
		*const_cast<ManagedRef<IAnnotation<TNote>>*>(&rNote) = std::move(*const_cast<ManagedRef<IAnnotation<TNote>>*>(&xObj.rNote));
		return *this;
	}

	virtual TString toString(void) const
	{ _toString(); }
};

#endif //Annotation_H