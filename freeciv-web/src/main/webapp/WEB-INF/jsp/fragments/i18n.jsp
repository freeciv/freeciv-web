<%@ page contentType="text/html; charset=UTF-8" %>
<%@ taglib prefix="fmt" uri="http://java.sun.com/jsp/jstl/fmt" %>
<fmt:requestEncoding value="UTF-8" />
<fmt:setLocale value="${not empty param.locale ? param.locale : pageContext.request.locale}" scope="session"/>
<fmt:setBundle basename="i18n/text"/>