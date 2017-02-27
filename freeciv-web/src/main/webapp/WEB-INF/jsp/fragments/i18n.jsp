<%@ page contentType="text/html; charset=UTF-8" %>
<%@ taglib prefix="fmt" uri="http://java.sun.com/jsp/jstl/fmt" %>
<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<fmt:requestEncoding value="UTF-8" />
<fmt:setLocale value="${not empty param.locale ? param.locale : pageContext.request.locale}" scope="session"/>
<c:set var="default_lang" scope="request" value="${empty param.locale or param.locale eq 'en_US' ? true : false}"/>
<fmt:setBundle basename="i18n/text"/>